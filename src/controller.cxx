#include "controller.hxx"
#include "exceptions.hxx"
#include <unistd.h>
#include <iostream>
#include <thread>
#include <memory>
#include <tbb/tbb.h>

using namespace ctrl;

Controller::Controller(p_conn_info_t conn_info,
                       unsigned int telnet_listen_port,
                       unsigned int http_listen_port)
  : conn_info(uptr_conn_info_t(conn_info)), tables_tries(uptr_table_tries_map_t(new table_tries_map_t())),
    telnet_listen_port(telnet_listen_port), http_listen_port(http_listen_port) {}

void Controller::workflow() {
  std::cout << "Defining pathways..." << std::endl;
  continue_node<continue_msg> start = continue_node<continue_msg>(tbb_graph,
                        [=](const continue_msg &v) { connect_to_database(); });
  continue_node<continue_msg> read_database = continue_node<continue_msg>(tbb_graph,
                        [=]( const continue_msg &v) { update_rate_data(); });
  make_edge(start, read_database);
  make_edge(read_database, read_database);
  std::cout << "Starting workflow..." << std::endl;
  start.try_put(continue_msg());
  tbb_graph.wait_for_all();
};

static uptr_controller_t controller;

p_controller_t Controller::get_controller(p_conn_info_t conn_info, unsigned int telnet_listen_port, unsigned int http_listen_port) {
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

void Controller::connect_to_database() {
  database = db::uptr_db_t(new db::DB(conn_info->host, conn_info->dbname, conn_info->user, conn_info->password, conn_info->port, conn_info->conn_count));
  set_new_rate_data();
}

void Controller::set_new_rate_data() {
  database->get_new_records();
}

void Controller::insert_new_rate_data(db::p_rate_record_t new_record) {
  tbb::mutex::scoped_lock lock(map_insertion_mutex);
  unsigned int new_rate_table_id = new_record->get_rate_table_id();
  table_tries_map_t::const_iterator it = tables_tries->find(new_rate_table_id);
  trie::p_trie_t trie;
  if (it == tables_tries->end()) {
     trie = new trie::Trie();
    (*tables_tries)[new_rate_table_id] = trie::uptr_trie_t(trie);
  }
  else {
    trie = (*tables_tries)[new_rate_table_id].get();
  }
  trie::p_trie_data_t new_data = new trie::TrieData();
  new_data->set_rate(new_record->get_rate());
  new_data->set_effective_date(new_record->get_effective_date());
  new_data->set_end_date(new_record->get_end_date());
  std::string str_prefix = new_record->get_prefix();
  size_t prefix_length = str_prefix.length();
  const char *prefix = str_prefix.c_str();
  trie::Trie::insert(trie, prefix, prefix_length, new_data);
}

void Controller::update_rate_data() {
  std::this_thread::sleep_for(std::chrono::seconds(10));
  //std::this_thread::sleep_for(std::chrono::hours(1));
  set_new_rate_data();
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
    unsigned int threads_number = 10;
    unsigned int telnet_listen_port = 23;
    unsigned int http_listen_port = 80;

    while ((opt = getopt(argc, argv, "c:d:u:p:s:n:t:w:h")) != -1) {
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
       default: /* '?' */
           fprintf(stderr, "Usage: %s [-h] [-c dbhost] [-d dbname] [-u dbuser] [-p dbpassword] [-s dbport] [-n threads_number ] [-t telnet_listen_port] [-w http_listen_port]\n\n", argv[0]);
           exit(EXIT_FAILURE);
       }
    }
    if (threads_number < 1) {
      std::cerr << "At least one thread is needed to run." << std::endl;
      exit(EXIT_FAILURE);
    }
    std::cout << "Starting..." << std::endl;
    tbb::task_scheduler_init init(threads_number);
    p_conn_info_t conn_info =  new ConnectionInfo();
    conn_info->host = dbhost;
    conn_info->dbname = dbname;
    conn_info->user = dbuser;
    conn_info->password = dbpassword;
    conn_info->port = dbport;
    conn_info->conn_count = threads_number;

    p_controller_t controller  = Controller::get_controller(conn_info, telnet_listen_port, http_listen_port);
    controller->workflow();
    return 0;
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
