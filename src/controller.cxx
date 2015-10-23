#include "controller.hxx"
#include "exceptions.hxx"
#include "rest.hxx"
#include "telnet.hxx"
#include <string>
#include <httpserver.hpp>
#include <iostream>
#include <thread>
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
      table_access_count = 0;
}

void Controller::clear_tables() {
  delete tables_index;
  for (size_t i = 0; i < tables_tries->size(); ++i)
    delete (*tables_tries)[i];
  tables_tries->clear();
  delete tables_tries;
  codes->clear();
  delete codes;
}

void Controller::renew_tables() {
  tables_index = new_tables_index;
  tables_tries = new_tables_tries;
  codes = new_codes;
}

void Controller::reset_new_tables() {
  new_tables_index = new trie::Trie();
  new_tables_tries = new trie::tries_t();
  new_codes = new codes_t();
}

void Controller::update_rate_tables_tries() {
  {
    std::lock_guard<std::mutex> update_lock(update_tables_mutex);
    updating_tables = true;
    mutex_unique_lock_t access_lock(access_tables_mutex);
    access_tables_holder.wait(access_lock, [&]{ return table_access_count == 0; });
    if (tables_tries)
      clear_tables();
    renew_tables();
    reset_new_tables();
    updating_tables = false;
    table_access_count = 0;
    access_lock.unlock();
  }
  update_tables_holder.notify_all();
}

void Controller::start_workflow() {
  tbb::task_group tasks;
  create_table_tries();
  tasks.run([&]{ run_http_server(); });
  tasks.run([&]{ run_telnet_server(); });
  update_table_tries();
  tasks.wait();
};

void Controller::create_table_tries() {
  database = new db::DB(*conn_info);
  reference_time = time(nullptr);
  database->get_new_records();
  update_rate_tables_tries();
}

void Controller::update_table_tries() {
  unsigned int refresh_minutes = database->get_refresh_minutes();
  while (true) {
    //insert_code_name_rate_table_db();
    time_t seconds = (refresh_minutes * 60) - (time(nullptr) - reference_time);
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
    reference_time = time(nullptr);
    database->get_new_records();
    update_rate_tables_tries();
  }
}

void Controller::insert_code_name_rate_table_db() {
  search::SearchResult result;
  std::cout << "Searching all codes..." << std::endl;
  for (unsigned char i = trie::rate_type_t::RATE_TYPE_DEFAULT; i <= trie::rate_type_t::RATE_TYPE_LOCAL; ++i)
    search_all_codes((trie::rate_type_t)i, result);
  std::cout << "Updating database with rate data..." << std::endl;
  //database->insert_code_name_rate_table_rate(result);
}

void Controller::run_http_server() {
  rest::Rest rest_server;
  rest_server.run_server(http_listen_port);
}

void Controller::run_telnet_server() {
  telnet::Telnet telnet_server;
  telnet_server.run_server(telnet_listen_port);
}


static p_controller_t controller;

p_controller_t Controller::get_controller(db::ConnectionInfo &conn_info, unsigned int telnet_listen_port, unsigned int http_listen_port) {
  if (!controller) {
    controller = new Controller(conn_info, telnet_listen_port, http_listen_port);
  }
  return controller;
}

p_controller_t Controller::get_controller() {
  if (!controller)
    throw ControllerNoInstanceException();
  return controller;
}

void Controller::insert_new_rate_data(const std::string &rate_table_id,
                                      unsigned int numeric_rate_table_id,
                                      const std::string &code,
                                      unsigned long long numeric_code,
                                      const std::string &code_name,
                                      double default_rate,
                                      double inter_rate,
                                      double intra_rate,
                                      double local_rate,
                                      time_t effective_date,
                                      time_t end_date,
                                      unsigned int egress_trunk_id) {
  size_t table_index;
  codes_t::iterator it = new_codes->end();
  {
    tbb::mutex::scoped_lock lock(map_insertion_mutex);
    if ((table_index = new_tables_index->search_table_index(new_tables_index, rate_table_id.c_str(), rate_table_id.size())) == 0) {
      new_tables_tries->push_back(new trie::Trie());
      table_index = new_tables_tries->size()-1;
      new_tables_index->insert_table_index(new_tables_index, rate_table_id.c_str(), rate_table_id.size(), table_index);
    }
    if ((it = new_codes->find(code_name)) == new_codes->end())
      (*new_codes)[code_name] = numeric_code;
  }
  if (it == new_codes->end())
    it = new_codes->find(code_name);
  p_code_pair_t code_name_ptr =  &(*it);
  trie::p_trie_t trie = (*new_tables_tries)[table_index];
  size_t code_length = code.length();
  const char *c_code = code.c_str();
  trie->insert_code(trie, c_code, code_length, code_name_ptr, numeric_rate_table_id, default_rate, inter_rate, intra_rate, local_rate, effective_date, end_date, reference_time, egress_trunk_id);
}

void Controller::search_code(const std::string &code, trie::rate_type_t rate_type, search::SearchResult &result) {
  unsigned long long numeric_code = std::stoull(code, nullptr, 10);
  if (numeric_code == 0 || std::to_string(numeric_code) != code)
    return;
  mutex_unique_lock_t update_tables_lock(update_tables_mutex);
  update_tables_holder.wait(update_tables_lock, [&] { return updating_tables == false; });
  update_tables_lock.unlock();
  _search_code(code, rate_type, result);
}

void Controller::search_code_name(const std::string &code_name, trie::rate_type_t rate_type, search::SearchResult &result) {
  mutex_unique_lock_t update_tables_lock(update_tables_mutex);
  update_tables_holder.wait(update_tables_lock, [&] { return updating_tables == false; });
  update_tables_lock.unlock();
  if (codes->find(code_name) == codes->end())
    return;
  unsigned long long numeric_code = (*codes)[code_name];
  if (numeric_code == 0)
    return;
  _search_code(std::to_string(numeric_code), rate_type, result);
}

void Controller::_search_code(const std::string &code, trie::rate_type_t rate_type, search::SearchResult &result) {
  /** THIS SHOULD BE NEVER CALLED DIRECTLY BECAUSE IT IS NOT THREAD SAFE */
  size_t code_length = code.length();
  const char* c_code = code.c_str();
  tbb::parallel_for(tbb::blocked_range<size_t>(0, tables_tries->size()),
    [&](const tbb::blocked_range<size_t> &r)  {
      for (auto i = r.begin(); i != r.end(); ++i) {
        {
          std::lock_guard<std::mutex> lock(access_tables_mutex);
          table_access_count++;
        }
        trie::p_trie_t trie = (*tables_tries)[i];
        trie->search_code(trie, c_code, code_length, rate_type, result);
        {
          std::lock_guard<std::mutex> lock(access_tables_mutex);
          table_access_count--;
        }
        access_tables_holder.notify_all();
      }
    }
  );
}

void Controller::search_code_name_rate_table(const std::string &code_name, const std::string &rate_table_id, trie::rate_type_t rate_type, search::SearchResult &result) {
  mutex_unique_lock_t update_tables_lock(update_tables_mutex);
  update_tables_holder.wait(update_tables_lock, [&] { return updating_tables == false; });
  update_tables_lock.unlock();
  if (codes->find(code_name) == codes->end())
    return;
  unsigned long long numeric_code = (*codes)[code_name];
  if (numeric_code == 0)
    return;
  size_t index;
  if ((index = tables_index->search_table_index(tables_index, rate_table_id.c_str(), rate_table_id.size())) == 0 )
    return;
  unsigned int numeric_rate_table_id = std::strtoul(rate_table_id.c_str(), nullptr, 10);
  if (std::to_string(numeric_rate_table_id) != rate_table_id)
    return;
  std::string s_code = std::to_string(numeric_code);
  const char* code = s_code.c_str();
  size_t code_length = s_code.size();
  {
    std::lock_guard<std::mutex> lock(access_tables_mutex);
    table_access_count++;
  }
  trie::p_trie_t trie = (*tables_tries)[index];
  trie->search_code(trie, code, code_length, rate_type, result);
  {
    std::lock_guard<std::mutex> lock(access_tables_mutex);
    table_access_count--;
  }
  access_tables_holder.notify_all();
}

void Controller::search_rate_table(const std::string &rate_table_id, trie::rate_type_t rate_type, search::SearchResult &result) {
  mutex_unique_lock_t update_tables_lock(update_tables_mutex);
  update_tables_holder.wait(update_tables_lock, [&] { return updating_tables == false; });
  update_tables_lock.unlock();
  size_t index;
  if ((index = tables_index->search_table_index(tables_index, rate_table_id.c_str(), rate_table_id.size())) == 0 )
    return;
  {
    std::lock_guard<std::mutex> lock(access_tables_mutex);
    table_access_count++;
  }
  //trie->total_search_code(trie, rate_type, result);
  std::vector<unsigned long long > all_codes;
  for (auto it = codes->begin(); it != codes->end(); ++it)
    all_codes.push_back(it->second);
  tbb::parallel_for(tbb::blocked_range<size_t>(0, all_codes.size()),
    [&](const tbb::blocked_range<size_t> &r){
      for (size_t i = r.begin(); i != r.end(); ++i) {
        trie::p_trie_t trie = (*tables_tries)[index];
        std::string code = std::to_string(all_codes[i]);
        trie->search_code(trie, code.c_str(), code.size(), rate_type, result);
      }
    });
  {
    std::lock_guard<std::mutex> lock(access_tables_mutex);
    table_access_count--;
  }
  access_tables_holder.notify_all();
}

void Controller::search_all_codes(trie::rate_type_t rate_type, search::SearchResult &result) {
  mutex_unique_lock_t update_tables_lock(update_tables_mutex);
  update_tables_holder.wait(update_tables_lock, [&] { return updating_tables == false; });
  update_tables_lock.unlock();
  std::vector<unsigned long long > all_codes;
  for (auto it = codes->begin(); it != codes->end(); ++it)
    all_codes.push_back(it->second);
  tbb::parallel_for (tbb::blocked_range2d<size_t, size_t>(0, tables_tries->size(), 0, all_codes.size()),
    [&](const tbb::blocked_range2d<size_t, size_t> &r)  {
      for (auto i = r.rows().begin(); i != r.rows().end(); ++i)
        for (auto j = r.cols().begin(); j != r.cols().end(); ++j) {
          {
            std::lock_guard<std::mutex> lock(access_tables_mutex);
            table_access_count++;
          }
          trie::p_trie_t trie = (*tables_tries)[i];
          std::string code = std::to_string(all_codes[j]);
          trie->search_code(trie, code.c_str(), code.size(), rate_type, result);
          {
            std::lock_guard<std::mutex> lock(access_tables_mutex);
            table_access_count--;
          }
        }
   });
}
