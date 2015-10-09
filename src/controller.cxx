#include "controller.hxx"
#include "exceptions.hxx"
#include "rest.hxx"
#include "telnet.hxx"
#include <string>
#include <httpserver.hpp>
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
      updating_tables = false;
}

void Controller::reset_new_tables() {
  new_rate_table_ids = uptr_rate_table_ids_t(new rate_table_ids_t());
  new_tables_tries = uptr_table_tries_map_t(new table_tries_map_t());
  new_code_names = uptr_code_names_t(new code_names_t());
  new_codes = uptr_codes_t(new codes_t());
}

void Controller::update_rate_tables_tries() {
  {
    std::lock_guard<std::mutex> update_lock(update_tables_mutex);
    updating_tables = true;
    mutex_unique_lock_t access_lock(access_tables_mutex);
    access_tables_holder.wait(access_lock, [&]{ return table_access_count == 0; });
    rate_table_ids = std::move(new_rate_table_ids);
    tables_tries = std::move(new_tables_tries);
    code_names = std::move(new_code_names);
    codes = std::move(new_codes);
    reset_new_tables();
    updating_tables = false;
    table_access_count = 0;
    access_lock.unlock();
  }
  update_tables_holder.notify_all();
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

void Controller::create_table_tries() {
  database = db::uptr_db_t(new db::DB(*conn_info));
  last_db_update = time(nullptr);
  database->get_new_records();
  update_rate_tables_tries();
}

void Controller::update_table_tries() {
  unsigned int refresh_minutes = database->get_refresh_minutes();
  while (true) {
    insert_code_name_rate_table_db();
    time_t seconds = (refresh_minutes * 60) - (time(nullptr) - last_db_update);
    if (seconds <= 0) {
      seconds = 0;
      std::cout << "Next update will be now since it took too long to load from database." << std::endl;
    }
    else if (seconds > 60) {
      unsigned int minutes = seconds / 60;
      std::cout << "Next update from the database will be in roughly " << minutes << (minutes > 1 ? " minutes." : " minute.") << std::endl;
    }
    else
      std::cout << "Next update from the database will be in " << seconds << " seconds." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    last_db_update = time(nullptr);
    database->get_new_records();
    update_rate_tables_tries();
  }
}

void Controller::insert_code_name_rate_table_db() {
  search::SearchResult result;
  search_all_codes(result);
  database->insert_code_name_rate_table_rate(result);
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
                                      unsigned long long numeric_code,
                                      std::string code_name,
                                      double rate,
                                      time_t effective_date,
                                      time_t end_date) {
  trie::p_trie_t trie;
  {
    tbb::mutex::scoped_lock lock(map_insertion_mutex);
    if (new_tables_tries->find(rate_table_id) == new_tables_tries->end()) {
      trie = new trie::Trie();
      (*new_tables_tries)[rate_table_id] = trie::uptr_trie_t(trie);
      new_rate_table_ids->push_back(rate_table_id);
    }
    if (new_code_names->find(numeric_code) == new_code_names->end())
      (*new_code_names)[numeric_code] = code_name;
    if (new_codes->find(code_name) == new_codes->end())
      (*new_codes)[code_name] = numeric_code;
  }
  trie = (*new_tables_tries)[rate_table_id].get();
  size_t code_length = code.length();
  const char *c_code = code.c_str();
  trie::Trie::insert(trie, c_code, code_length, rate, effective_date, end_date);
}

void Controller::search_code(std::string code, search::SearchResult &result) {
  unsigned long long numeric_code = std::stoull(code, nullptr, 10);
  if (numeric_code == 0 || std::to_string(numeric_code) != code)
    return;
  mutex_unique_lock_t update_tables_lock(update_tables_mutex);
  update_tables_holder.wait(update_tables_lock, [&] { return updating_tables == false; });
  update_tables_lock.unlock();
  _search_code(code, result);
}

void Controller::search_code_name(std::string code_name, search::SearchResult &result) {
  mutex_unique_lock_t update_tables_lock(update_tables_mutex);
  update_tables_holder.wait(update_tables_lock, [&] { return updating_tables == false; });
  update_tables_lock.unlock();
  if (codes->find(code_name) == codes->end())
    return;
  unsigned long long numeric_code = (*codes)[code_name];
  if (numeric_code == 0)
    return;
  _search_code(std::to_string(numeric_code), result);
}

void Controller::_search_code(std::string code, search::SearchResult &result) {
  /** THIS SHOULD BE NEVER CALLED DIRECTLY BECAUSE IT IS NOT THREAD SAFE */
  size_t code_length = code.length();
  const char* c_code = code.c_str();
  ctrl::p_code_names_t p_code_names = code_names.get();
  parallel_for(tbb::blocked_range<size_t>(0, tables_tries->size()),
    [&](const tbb::blocked_range<size_t> &r)  {
      for (auto i = r.begin(); i != r.end(); ++i) {
        unsigned int rate_table_id = (*rate_table_ids)[i];
        {
          std::lock_guard<std::mutex> lock(access_tables_mutex);
          table_access_count++;
        }
        trie::p_trie_t trie = (*tables_tries)[rate_table_id].get();
        trie->search(trie, c_code, code_length, rate_table_id, p_code_names, result);
        {
          std::lock_guard<std::mutex> lock(access_tables_mutex);
          table_access_count--;
        }
        access_tables_holder.notify_all();
      }
    }
  );
}

void Controller::search_code_name_rate_table(std::string code_name, unsigned int rate_table_id, search::SearchResult &result) {
  mutex_unique_lock_t update_tables_lock(update_tables_mutex);
  update_tables_holder.wait(update_tables_lock, [&] { return updating_tables == false; });
  update_tables_lock.unlock();
  if (codes->find(code_name) == codes->end())
    return;
  unsigned long long numeric_code = (*codes)[code_name];
  if (numeric_code == 0)
    return;
  if (tables_tries->find(rate_table_id) == tables_tries->end())
    return;
  ctrl::p_code_names_t p_code_names = code_names.get();
  std::string s_code = std::to_string(numeric_code);
  const char* code = s_code.c_str();
  size_t code_length = s_code.size();
  {
    std::lock_guard<std::mutex> lock(access_tables_mutex);
    table_access_count++;
  }
  trie::p_trie_t trie = (*tables_tries)[rate_table_id].get();
  trie->search(trie, code, code_length, rate_table_id, p_code_names, result);
  {
    std::lock_guard<std::mutex> lock(access_tables_mutex);
    table_access_count--;
  }
  access_tables_holder.notify_all();
}

void Controller::search_rate_table(unsigned int rate_table_id, search::SearchResult &result) {
  mutex_unique_lock_t update_tables_lock(update_tables_mutex);
  update_tables_holder.wait(update_tables_lock, [&] { return updating_tables == false; });
  update_tables_lock.unlock();
  if (tables_tries->find(rate_table_id) == tables_tries->end())
    return;
  ctrl::p_code_names_t p_code_names = code_names.get();
  {
    std::lock_guard<std::mutex> lock(access_tables_mutex);
    table_access_count++;
  }
  trie::p_trie_t trie = (*tables_tries)[rate_table_id].get();
  trie->total_search(trie, rate_table_id, p_code_names, result);
  {
    std::lock_guard<std::mutex> lock(access_tables_mutex);
    table_access_count--;
  }
  access_tables_holder.notify_all();
}

void Controller::search_all_codes(search::SearchResult &result) {
  mutex_unique_lock_t update_tables_lock(update_tables_mutex);
  update_tables_holder.wait(update_tables_lock, [&] { return updating_tables == false; });
  update_tables_lock.unlock();
  std::vector<std::pair<std::string, unsigned int>> code_name_rate_table;
  for (auto i = codes->begin(); i != codes->end(); ++i)
    for (auto j = tables_tries->begin(); j != tables_tries->end(); ++j)
      code_name_rate_table.push_back(std::make_pair(i->first, j->first));
  parallel_for (tbb::blocked_range<size_t>(0, code_name_rate_table.size()),
    [&](const tbb::blocked_range<size_t> &r)  {
      for (auto i = r.begin(); i != r.end(); ++i) {
        std::pair<std::string, unsigned int> pair = code_name_rate_table[i];
        std::string code_name = pair.first;
        unsigned int rate_table_id = pair.second;
        {
          std::lock_guard<std::mutex> lock(access_tables_mutex);
          table_access_count++;
        }
        search_code_name_rate_table(code_name, rate_table_id, result);
        {
          std::lock_guard<std::mutex> lock(access_tables_mutex);
          table_access_count--;
        }
        access_tables_holder.notify_all();
      }
    });
}
