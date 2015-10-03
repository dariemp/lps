#include "controller.hxx"
#include "exceptions.hxx"
#include "rest.hxx"
#include "telnet.hxx"
#include <string>
#include <httpserver.hpp>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <memory>
#include <queue>
#include <tbb/tbb.h>
#include <tbb/flow_graph.h>

using namespace ctrl;
using namespace tbb::flow;

Controller::Controller(db::ConnectionInfo &conn_info,
                       unsigned int telnet_listen_port,
                       unsigned int http_listen_port)
  : conn_info(&conn_info), telnet_listen_port(telnet_listen_port), http_listen_port(http_listen_port) {
      reset_new_tables();
}

void Controller::reset_new_tables() {
  new_rate_table_ids = uptr_rate_table_ids_t(new rate_table_ids_t());
  new_tables_tries = uptr_table_tries_map_t(new table_tries_map_t());
  updating_tables = false;
}

void Controller::start_workflow() {
  std::cout << "Defining pathways..." << std::endl;
  graph tbb_graph;
  continue_node<continue_msg> start_node (tbb_graph, [=](const continue_msg &) { create_table_tries(); });
  continue_node<continue_msg> update_node (tbb_graph, [=]( const continue_msg &) { update_table_tries(); });
  continue_node<continue_msg> http_node (tbb_graph, [=]( const continue_msg &) { run_http_server(); });
  continue_node<continue_msg> telnet_node (tbb_graph, [=]( const continue_msg &) { run_telnet_server(); });
  make_edge(start_node, update_node);
  make_edge(start_node, http_node);
  make_edge(start_node, telnet_node);
  std::cout << "Starting workflow..." << std::endl;
  start_node.try_put(continue_msg());
  tbb_graph.wait_for_all();
};

void Controller::update_rate_tables_tries() {
  std::lock_guard<std::mutex> threads_guard_lock(update_tables_mutex);
  updating_tables = true;
  rate_table_ids = std::move(new_rate_table_ids);
  tables_tries = std::move(new_tables_tries);
  reset_new_tables();
  condition_variable_tables.notify_all();
}

void Controller::create_table_tries() {
  database = db::uptr_db_t(new db::DB(*conn_info));
  database->get_new_records();
  update_rate_tables_tries();
}

void Controller::update_table_tries() {
  unsigned int refresh_minutes = database->get_refresh_minutes();
  while (true) {
    std::cout << "Next update from the database in " << refresh_minutes << (refresh_minutes == 1 ? " minute." : " minutes.") << std::endl;
    std::this_thread::sleep_for(std::chrono::minutes(refresh_minutes));
    database->get_new_records();
    update_rate_tables_tries();
  }
}

void Controller::run_http_server() {
  rest::Rest rest_server;
  rest_server.run_server(http_listen_port);
}

void Controller::run_telnet_server() {
  telnet::Telnet telnet_server;
  telnet_server.run_server(telnet_listen_port);
}


static uptr_controller_t controller;

p_controller_t Controller::get_controller(db::ConnectionInfo &conn_info, unsigned int telnet_listen_port, unsigned int http_listen_port) {
  p_controller_t instance = controller.get();
  if (!instance) {
    instance = new Controller(conn_info, telnet_listen_port, http_listen_port);
    controller = uptr_controller_t(instance);
  }
  return instance;
}

p_controller_t Controller::get_controller() {
  p_controller_t instance = controller.get();
  if (!instance)
    throw ControllerNoInstanceException();
  return instance;
}

void Controller::insert_new_rate_data(unsigned int rate_table_id,
                                      std::string code,
                                      double rate,
                                      time_t effective_date,
                                      time_t end_date) {
  tbb::mutex::scoped_lock lock(map_insertion_mutex);
  table_tries_map_t::const_iterator it = new_tables_tries->find(rate_table_id);
  trie::p_trie_t trie;
  if (it == new_tables_tries->end()) {
    trie = new trie::Trie();
    (*new_tables_tries)[rate_table_id] = trie::uptr_trie_t(trie);
    new_rate_table_ids->push_back(rate_table_id);
  }
  trie = (*new_tables_tries)[rate_table_id].get();
  size_t code_length = code.length();
  const char *c_code = code.c_str();
  trie::Trie::insert(trie, c_code, code_length, rate, effective_date, end_date);
}

void Controller::search_code(std::string code, search::SearchResult &result) {
  unsigned long long number_code  = std::stoull(code, nullptr, 10);
  if (number_code == 0 || std::to_string(number_code) != code)
    return;
  std::unique_lock<std::mutex> threads_wait_lock(update_tables_mutex);
  condition_variable_tables.wait(threads_wait_lock, [] { return updating_tables == false; });
  size_t code_length = code.length();
  const char* c_code = code.c_str();
  std::vector<unsigned int> keys;
  parallel_for(tbb::blocked_range<size_t>(0, tables_tries->size(), 1),
    [&](const tbb::blocked_range<size_t> &r)  {
        size_t index = r.begin();
        unsigned int rate_table_id = (*rate_table_ids)[index];
        //output_mutex.lock();
        //std::cout << "Searching rate table: " << rate_table_id << std::endl;
        trie::p_trie_t trie = (*tables_tries)[rate_table_id].get();
        trie::p_trie_data_t trie_data = trie->search(trie, c_code, code_length);
        //output_mutex.unlock();
        if (trie_data)
          result.insert(rate_table_id,
                        trie_data->get_rate(),
                        trie_data->get_effective_date(),
                        trie_data->get_end_date());
    }
  );
}

int main(int argc, char *argv[]) {
  try {
    std::cout.setf( std::ios_base::unitbuf );
    int opt;
    std::string dbhost = "127.0.0.1";
    std::string dbname = "icxp";
    std::string dbuser = "class4";
    std::string dbpassword= "class4";
    unsigned int dbport = 5432;
    unsigned int threads_number = 11;
    unsigned int telnet_listen_port = 23;
    unsigned int http_listen_port = 80;
    unsigned int rows_to_read_debug = 0;
    unsigned int refresh_minutes = 30;

    while ((opt = getopt(argc, argv, "c:d:u:p:s:n:t:w:r:m:h")) != -1) {
       switch (opt) {
       case 'c':
          dbhost = std::string(optarg);
          break;
       case 'd':
          dbname = std::string(optarg);
          break;
       case 'u':
          dbuser = std::string(optarg);
          break;
       case 'p':
          dbpassword = std::string(optarg);
          break;
       case 's':
          dbport = atoi(optarg);
          break;
       case 'n':
          threads_number = atoi(optarg);
          break;
       case 't':
          telnet_listen_port = atoi(optarg);
          break;
       case 'w':
          http_listen_port = atoi(optarg);
          break;
       case 'r':
          rows_to_read_debug = atoi(optarg);
          break;
       case 'm':
          refresh_minutes = atoi(optarg);
          break;
       default: /* '?' */
           std::cerr << "Usage: " << argv[0] << " [-h] [-c dbhost] [-d dbname] [-u dbuser] [-p dbpassword]" << std::endl;
           std::cerr << "          [-s dbport] [-t telnet_listen_port] [-w http_listen_port]" << std::endl;
           std::cerr << "          [-n threads_number ] [-r rows_to_read_debug] [-m refresh_minutes]" << std::endl;
           exit(EXIT_FAILURE);
       }
    }
    if (threads_number < 1) {
      std::cerr << "At least one thread is needed to run." << std::endl;
      exit(EXIT_FAILURE);
    }
    std::cout << "Starting..." << std::endl;
    tbb::task_scheduler_init init(threads_number);
    //p_conn_info_t conn_info =  new ConnectionInfo();
    db::ConnectionInfo conn_info;
    conn_info.host = dbhost;
    conn_info.dbname = dbname;
    conn_info.user = dbuser;
    conn_info.password = dbpassword;
    conn_info.port = dbport;
    conn_info.conn_count = threads_number - 1;
    conn_info.rows_to_read_debug = rows_to_read_debug;
    conn_info.refresh_minutes = refresh_minutes;

    p_controller_t controller  = Controller::get_controller(conn_info, telnet_listen_port, http_listen_port);
    controller->start_workflow();
    return 0;
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
