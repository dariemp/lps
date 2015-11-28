#include "controller.hxx"
#include "exceptions.hxx"
#include "rest.hxx"
#include "telnet.hxx"
#include "code.hxx"
#include "logger.hxx"
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
  : conn_info(&conn_info),
    world_tables_tries(nullptr),
    us_tables_tries(nullptr),
    az_tables_tries(nullptr),
    codes(nullptr),
    old_world_tables_tries(nullptr),
    old_us_tables_tries(nullptr),
    old_az_tables_tries(nullptr),
    old_codes(nullptr),
    telnet_listen_port(telnet_listen_port),
    http_listen_port(http_listen_port),
    updating_tables(false),
    clearing_tables(false)
{
  reset_new_tables();
  for (size_t i = 0; i < conn_info.conn_count; ++i) {
    tries_release_queues.push_back(new trie_release_queue_t());
    codes_release_queues.push_back(new code_release_queue_t());
  }
}

Controller::~Controller() {
  clear_tables();
  for (size_t i = 0; i < tries_release_queues.size(); ++i) {
    delete tries_release_queues[i];
    codes_release_queues[i]->clear();
    delete codes_release_queues[i];
  }
  tries_release_queues.clear();
  codes_release_queues.clear();
}

void Controller::clear_tables() {
  log("Clearing tables...will push tries to release queues...");
  clearing_tables = true;
  for (auto it = old_world_tables_tries->begin(); it != old_world_tables_tries->end(); ++it) {
    unsigned int worker_index = (*it)->get_worker_index();
    p_trie_release_queue_t trie_release_queue = tries_release_queues[worker_index];
    trie_release_queue->push(*it);
  }
  for (auto it = old_us_tables_tries->begin(); it != old_us_tables_tries->end(); ++it) {
    unsigned int worker_index = (*it)->get_worker_index();
    p_trie_release_queue_t trie_release_queue = tries_release_queues[worker_index];
    trie_release_queue->push(*it);
  }
  for (auto it = old_az_tables_tries->begin(); it != old_az_tables_tries->end(); ++it) {
    unsigned int worker_index = (*it)->get_worker_index();
    p_trie_release_queue_t trie_release_queue = tries_release_queues[worker_index];
    trie_release_queue->push(*it);
  }
  log("Tries pushed... releasing vectors...");
  old_world_tables_tries->clear();
  old_us_tables_tries->clear();
  old_az_tables_tries->clear();
  delete old_world_tables_tries;
  delete old_us_tables_tries;
  delete old_az_tables_tries;
  old_world_tables_index->clear();
  old_us_tables_index->clear();
  old_az_tables_index->clear();
  delete old_world_tables_index;
  delete old_us_tables_index;
  delete old_az_tables_index;
  log("Vectors released.... realeasing codes...");
  for (auto it = old_codes->begin(); it != old_codes->end(); ++it) {
    p_code_value_t code_value = it->second;
    unsigned int worker_index = code_value->first;
    p_code_release_queue_t code_release_queue = codes_release_queues[worker_index];
    code_release_queue->push(code_value);
  }
  old_codes->clear();
  delete old_codes;
  clearing_tables = false;
  log("Tables cleared.");
}

void Controller::renew_tables() {
  old_world_tables_index = world_tables_index;
  old_world_tables_tries = world_tables_tries;
  old_us_tables_index = us_tables_index;
  old_us_tables_tries = us_tables_tries;
  old_az_tables_index = az_tables_index;
  old_az_tables_tries = az_tables_tries;
  old_codes = codes;
  world_tables_index = new_world_tables_index;
  world_tables_tries = new_world_tables_tries;
  us_tables_index = new_us_tables_index;
  us_tables_tries = new_us_tables_tries;
  az_tables_index = new_az_tables_index;
  az_tables_tries = new_az_tables_tries;
  codes = new_codes;
}

void Controller::reset_new_tables() {
  new_world_tables_index = new tables_index_t();
  new_world_tables_tries = new tables_tries_t();
  new_us_tables_index = new tables_index_t();
  new_us_tables_tries = new tables_tries_t();
  new_az_tables_index = new tables_index_t();
  new_az_tables_tries = new tables_tries_t();
  new_codes = new codes_t();
}

void Controller::update_rate_tables_tries() {
  {
    database->wait_for_reading();
    log("Updating rate tables...");
    std::lock_guard<std::mutex> update_lock(update_tables_mutex);
    updating_tables = true;
    if (old_world_tables_tries || old_us_tables_tries || old_az_tables_tries || old_codes)
      clear_tables();
    renew_tables();
    reset_new_tables();
    updating_tables = false;
  }
  update_tables_holder.notify_all();
}

void Controller::start_workflow() {
  tbb::task_group tasks;
  tasks.run([&]{ run_logger(); });
  tasks.run([&]{ run_http_server(); });
  tasks.run([&]{ run_telnet_server(); });
  for (size_t worker_index = 0; worker_index < conn_info->conn_count; ++worker_index)
    tasks.run([&, worker_index]{ run_worker(worker_index); });
  create_table_tries();
  update_table_tries();
  tasks.wait();
};

void Controller::run_worker(unsigned int worker_index) {
  p_trie_release_queue_t worker_trie_release_queue = tries_release_queues[worker_index];
  p_code_release_queue_t worker_code_release_queue = codes_release_queues[worker_index];
  while (true) {
    while (!(clearing_tables || (database != nullptr && database->is_reading())))
      std::this_thread::sleep_for(std::chrono::seconds(1));
    if (database->is_reading())
      database->read_chunk(worker_index);
    else {
      while (clearing_tables || !(worker_trie_release_queue->empty() && worker_code_release_queue->empty())) {
        trie::p_trie_t some_trie;
        p_code_value_t some_code_value;
        while (worker_trie_release_queue->try_pop(some_trie)) {
          if (some_trie->get_worker_index() != worker_index)
            throw WorkerReleaseLeakException();
          for (size_t i = 0; i < 10; ++i)
            if (some_trie->has_child(i)) {
              trie::p_trie_t some_child = some_trie->get_child(i);
              unsigned int child_creator_worker = some_child->get_worker_index();
              p_trie_release_queue_t designated_worker_queue = tries_release_queues[child_creator_worker];
              designated_worker_queue->push(some_child);
            }
          delete some_trie;
        }
        while (worker_code_release_queue->try_pop(some_code_value)) {
          if (some_code_value->first != worker_index)
            throw WorkerReleaseLeakException();
          some_code_value->second->clear();
          delete some_code_value;
        }
      }
    } // else
  }
}

void Controller::run_logger() {
  while (true) {
    std::string output = "";
    std::string errput = "";
    bool output_available = try_get_output(output);
    bool errput_available = try_get_error(errput);
    if  (!(output_available || errput_available))
      std::this_thread::sleep_for(std::chrono::seconds(1));
    if (output != "")
      std::cout << output << std::endl;
    if (errput != "")
      std::cerr << errput << std::endl;
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

void Controller::create_table_tries() {
  database = new db::DB(*conn_info);
  reference_time = time(nullptr);
  database->init_load_cicle(reference_time);
  update_rate_tables_tries();
}

void Controller::update_table_tries() {
  while (true) {
    database->wait_till_next_load_cicle();
    reference_time = time(nullptr);
    database->init_load_cicle(reference_time);
    update_rate_tables_tries();
  }
}

void Controller::insert_code_name_rate_table_db() {
  search::SearchResult result;
  log("Searching all codes...");
  for (unsigned char i = trie::rate_type_t::RATE_TYPE_DEFAULT; i <= trie::rate_type_t::RATE_TYPE_LOCAL; ++i)
    search_all_codes((trie::rate_type_t)i, result);
  log("Updating database with rate data...");
  //database->insert_code_name_rate_table_rate(result);
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

Controller::table_trie_set_t Controller::select_table_trie(unsigned long long code, const std::string &code_name, bool inserting) {
  table_trie_set_t result;
  trie::Code dyn_code(code);
  char first_digit = dyn_code.next_digit();
  if (first_digit != 1) {
    if (inserting) {
      result.tables_tries = new_world_tables_tries;
      result.tables_index = new_world_tables_index;
      result.table_index_insertion_mutex = &world_table_index_insertion_mutex;
    }
    else {
      result.tables_tries = world_tables_tries;
      result.tables_index = world_tables_index;
      result.table_index_insertion_mutex = nullptr;
    }
  }
  else if (code_name == "USA" || code_name == "UNITED STATES") {
    if (inserting) {
      result.tables_tries = new_us_tables_tries;
      result.tables_index = new_us_tables_index;
      result.table_index_insertion_mutex = &us_table_index_insertion_mutex;
    }
    else {
      result.tables_tries = us_tables_tries;
      result.tables_index = us_tables_index;
      result.table_index_insertion_mutex = nullptr;
    }
  }
  else {
    if (inserting) {
      result.tables_tries = new_az_tables_tries;
      result.tables_index = new_az_tables_index;
      result.table_index_insertion_mutex = &az_table_index_insertion_mutex;
    }
    else {
      result.tables_tries = az_tables_tries;
      result.tables_index = az_tables_index;
      result.table_index_insertion_mutex = nullptr;
    }
  }
  return result;
}

void Controller::insert_new_rate_data(db::db_data_t db_data) {
  codes_t::iterator it = new_codes->end();
  str_to_upper(db_data.code_name);
  {
    tbb::mutex::scoped_lock lock(code_name_creation_mutex);
    if ((it = new_codes->find(db_data.code_name)) == new_codes->end())
      (*new_codes)[db_data.code_name] = new code_value_t(db_data.conn_index, new code_set_t());
    (*new_codes)[db_data.code_name]->second->insert(db_data.code);
  }
  if (it == new_codes->end())
    it = new_codes->find(db_data.code_name);
  p_code_pair_t code_items =  &(*it);
  table_trie_set_t selected_tables = select_table_trie(db_data.code, db_data.code_name, true);
  {
    tbb::mutex::scoped_lock lock(*selected_tables.table_index_insertion_mutex);
    if (selected_tables.tables_index->find(db_data.rate_table_id) == selected_tables.tables_index->end()) {
      selected_tables.tables_tries->push_back(new trie::Trie(db_data.conn_index));
      (*selected_tables.tables_index)[db_data.rate_table_id] = selected_tables.tables_tries->size()-1;
    }
  }
  size_t index = (*selected_tables.tables_index)[db_data.rate_table_id];
  trie::p_trie_t trie = (*selected_tables.tables_tries)[index];
  {
    tbb::mutex::scoped_lock lock(*trie->get_mutex());
    trie->insert_code(trie, db_data.conn_index, db_data.code, code_items, db_data.rate_table_id, db_data.default_rate, db_data.inter_rate, db_data.intra_rate, db_data.local_rate, db_data.effective_date, db_data.end_date, reference_time, db_data.egress_trunk_id);
  }
}

bool Controller::are_tables_available() {
  return world_tables_index != nullptr && world_tables_tries != nullptr &&
         us_tables_index != nullptr && us_tables_tries != nullptr &&
         az_tables_index != nullptr && az_tables_tries != nullptr &&
         codes != nullptr;
}

void Controller::search_code(unsigned long long code, trie::rate_type_t rate_type, search::SearchResult &result) {
  mutex_unique_lock_t update_tables_lock(update_tables_mutex);
  update_tables_holder.wait(update_tables_lock, [&] { return updating_tables == false; });
  update_tables_lock.unlock();
  if (!are_tables_available())
    return;
  table_trie_set_t selected_tables;
  if (code < 1000)
    selected_tables = select_table_trie(code, "USA", false); // Code name is only used if code first digit is 1.
  else
    selected_tables = select_table_trie(code, "", false);    // Code name is ignored.
  code_set_t code_set;
  code_set.insert(code);
  _search_code(code_set, rate_type, selected_tables, result);
  code_set.clear();
}

void Controller::search_code_name(std::string &code_name, trie::rate_type_t rate_type, search::SearchResult &result) {
  mutex_unique_lock_t update_tables_lock(update_tables_mutex);
  update_tables_holder.wait(update_tables_lock, [&] { return updating_tables == false; });
  update_tables_lock.unlock();
  if (!are_tables_available())
    return;
  str_to_upper(code_name);
  if (codes->find(code_name) == codes->end())
    return;
  p_code_set_t code_set = (*codes)[code_name]->second;
  table_trie_set_t selected_tables = select_table_trie(*code_set->begin(), code_name, false);
  _search_code(*code_set, rate_type, selected_tables, result, code_name);
}

void Controller::_search_code(const code_set_t &code_set, trie::rate_type_t rate_type, table_trie_set_t selected_tables, search::SearchResult &result, const std::string &filter_code_name) {
  /** THIS SHOULD BE NEVER CALLED DIRECTLY BECAUSE IT IS NOT THREAD SAFE */
  tbb::parallel_for(tbb::blocked_range<size_t>(0, selected_tables.tables_tries->size()),
    [&](const tbb::blocked_range<size_t> &r)  {
        for( size_t i = r.begin(); i != r.end(); ++i ) {
          trie::p_trie_t trie = (*selected_tables.tables_tries)[i];
          for (auto it = code_set.begin(); it != code_set.end(); ++it) {
            unsigned long long code = *it;
            trie->search_code(trie, code, rate_type, reference_time, result, filter_code_name);
          }
      }
    }
  );
}

void Controller::search_code_name_rate_table(std::string &code_name, unsigned int rate_table_id, trie::rate_type_t rate_type, search::SearchResult &result, bool include_code) {
  mutex_unique_lock_t update_tables_lock(update_tables_mutex);
  update_tables_holder.wait(update_tables_lock, [&] { return updating_tables == false; });
  update_tables_lock.unlock();
  if (!are_tables_available())
    return;
  str_to_upper(code_name);
  if (codes->find(code_name) == codes->end())
    return;
  p_code_set_t code_set = (*codes)[code_name]->second;
  table_trie_set_t selected_tables = select_table_trie(*code_set->begin(), code_name, false);
  if (selected_tables.tables_index->find(rate_table_id) == selected_tables.tables_index->end())
    return;
  size_t index = (*selected_tables.tables_index)[rate_table_id];
  trie::p_trie_t trie = (*selected_tables.tables_tries)[index];
  for (auto it = code_set->begin(); it != code_set->end(); ++it) {
    unsigned long long code = *it;
    trie->search_code(trie, code, rate_type, reference_time, result, code_name, include_code);
  }
}

void Controller::search_rate_table(unsigned int rate_table_id, trie::rate_type_t rate_type, search::SearchResult &result) {
  mutex_unique_lock_t update_tables_lock(update_tables_mutex);
  update_tables_holder.wait(update_tables_lock, [&] { return updating_tables == false; });
  update_tables_lock.unlock();
  p_tables_index_t tables_index;
  p_tables_tries_t tables_tries;
  if (!are_tables_available())
    return;
  std::vector<p_code_set_t> all_codes_sets;
  for (auto it = codes->begin(); it != codes->end(); ++it)
    all_codes_sets.push_back(it->second->second);
  tbb::parallel_for(tbb::blocked_range2d<size_t, size_t>(0, 3, 0, all_codes_sets.size()),
    [&](const tbb::blocked_range2d<size_t, size_t> &r){
      for (size_t i = r.rows().begin(); i != r.rows().end(); ++i) {
        switch (i) {
          case 0:
            tables_index = world_tables_index;
            tables_tries = world_tables_tries;
            break;
          case 1:
            tables_index = us_tables_index;
            tables_tries = us_tables_tries;
            break;
          case 2:
            tables_index = az_tables_index;
            tables_tries = az_tables_tries;
          break;
        }
        if (tables_index->find(rate_table_id) == tables_index->end())
          continue;
        size_t index = (*tables_index)[rate_table_id];
        trie::p_trie_t trie = (*tables_tries)[index];
        for (size_t j = r.cols().begin(); j != r.cols().end(); ++j) {
          p_code_set_t code_set = all_codes_sets[j];
          for (auto it = code_set->begin(); it != code_set->end(); ++it) {
            unsigned long long code = *it;
            trie->search_code(trie, code, rate_type, reference_time, result);
          }
        }
      }
    });
  all_codes_sets.clear();
}

void Controller::search_all_codes(trie::rate_type_t rate_type, search::SearchResult &result) {
  mutex_unique_lock_t update_tables_lock(update_tables_mutex);
  update_tables_holder.wait(update_tables_lock, [&] { return updating_tables == false; });
  update_tables_lock.unlock();
  if (!are_tables_available())
    return;
  //p_tables_index_t tables_index;
  p_tables_tries_t tables_tries;
  std::set<unsigned long long > all_codes;
  for (auto it = codes->begin(); it != codes->end(); ++it) {
    p_code_set_t code_set = it->second->second;
    all_codes.insert(code_set->begin(), code_set->end());
  }
  /*tbb::parallel_for(tbb::blocked_range<size_t>(0, 3),
    [&](const tbb::blocked_range<size_t> &r)  {
      for (auto i = r.begin(); i != r.end(); ++i) {
        switch (i) {
          case 0:
            tables_index = world_tables_index;
            tables_tries = world_tables_tries;
            break;
          case 1:
            tables_index = us_tables_index;
            tables_tries = us_tables_tries;
            break;
          case 2:*/
            //tables_index = az_tables_index;
            tables_tries = az_tables_tries;
          /*break;
        }*/
        tbb::parallel_for(tbb::blocked_range<size_t>(0, tables_tries->size()),
          [&](const tbb::blocked_range<size_t> &s)  {
            for (auto j = s.begin(); j != s.end(); ++j) {
              trie::p_trie_t trie = (*tables_tries)[j];
              for (auto it=all_codes.begin(); it != all_codes.end(); ++it) {
                unsigned long long code = *it;
                trie->search_code(trie, code, rate_type, reference_time, result);
              }
            }
          });
      //}
   //});
   all_codes.clear();
}
