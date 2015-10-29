#include "db.hxx"
#include "exceptions.hxx"
#include "controller.hxx"
#include <tbb/tbb.h>
#include <string>
#include <iostream>
#include <atomic>
#include <thread>

using namespace db;


DB::DB(ConnectionInfo &conn_info) {
  p_conn_info = &conn_info;
  if (conn_info.conn_count < 1)
    throw DBNoConnectionsException();
  std::cout << "Connecting to database with " << conn_info.conn_count << " connections... ";
  tbb::parallel_for(tbb::blocked_range<size_t>(0, conn_info.conn_count),
      [=](const tbb::blocked_range<size_t>& r) {
          for (size_t i = r.begin(); i != r.end(); ++i)
            connections.push_back(new pqxx::connection("host=" + conn_info.host + " dbname=" + conn_info.dbname + " user=" + conn_info.user + " password=" + conn_info.password + " port=" + std::to_string(conn_info.port)));
      });
  std::cout << "done." << std::endl;
}

DB::~DB() {
  for (size_t i = 0; i < connections.size(); ++i)
    delete connections[i];
  connections.clear();
}

unsigned int DB::get_refresh_minutes() {
  return p_conn_info->refresh_minutes;
}

unsigned int DB::get_first_rate_id(bool from_beginning) {
  pqxx::work transaction(*connections[0]);
  std::string order = from_beginning ? "" : "desc ";
  pqxx::result result = transaction.exec("select rate_id from rate order by rate_id " + order + "limit 1");
  transaction.commit();
  unsigned int ret_val;
  if (result.empty())
    ret_val = 0;
  else
    ret_val = result.begin()[0].as<unsigned int>();
  result.clear();
  return ret_val;
}

void DB::get_new_records() {
  std::cout << "Loading rate records from the database... ";
  unsigned int first_rate_id;
  unsigned int last_rate_id;
  if (p_conn_info->first_row_to_read_debug)
    first_rate_id = p_conn_info->first_row_to_read_debug;
  else
    first_rate_id = get_first_rate_id();
  if (p_conn_info->last_row_to_read_debug)
    last_rate_id = p_conn_info->last_row_to_read_debug;
  else
    last_rate_id = get_first_rate_id(false);
  unsigned int conn_count = connections.size();
  if (conn_count == 1) {
    std::cout << std::endl;
    query_database(0, "all", first_rate_id, last_rate_id);
  }
  else {
    std::cout << "in parallel... " << std::endl;
    std::cout << "DB rate first_rate_id: " << first_rate_id << std::endl;
    std::cout << "DB rate last_rate_id: " << last_rate_id << std::endl;
    unsigned int chunk_size = p_conn_info->chunk_size;
    std::atomic_uint last_queried_row;
    last_queried_row = first_rate_id - 1;
    tbb::mutex range_selection_mutex;
    tbb::task_group db_tasks;
    std::atomic_uint tasks_running;
    tasks_running = conn_count;
    for (size_t conn_index = 0; conn_index < conn_count; ++conn_index)
      db_tasks.run([&, conn_index]{
        unsigned int range_first_rate_id;
        unsigned int range_last_rate_id;
        while (last_queried_row < last_rate_id) {
          {
            tbb::mutex::scoped_lock lock(range_selection_mutex);
            range_first_rate_id = last_queried_row + 1;
            range_last_rate_id = range_first_rate_id + chunk_size;
            if (range_last_rate_id > last_rate_id)
              range_last_rate_id = last_rate_id;
            last_queried_row = range_last_rate_id;
          }
          query_database(conn_index, std::to_string(chunk_size), range_first_rate_id, range_last_rate_id);
        }
        tasks_running--;
      });
    ctrl::p_controller_t controller = ctrl::Controller::get_controller();
    while (tasks_running) {
      db_data_t db_data;
      if (db_queue.try_pop(db_data))
        controller->insert_new_rate_data(db_data);
    }
    db_tasks.wait();
  }
  std::cout << "done." << std::endl;
}

void DB::query_database(unsigned int conn_index, std::string chunk_size, int first_rate_id, unsigned int last_rate_id) {
  int remaining_retries = 3;
  while (remaining_retries > 0) {
    try {
      pqxx::work transaction(*connections[conn_index]);
      {
        tbb::mutex::scoped_lock lock(track_output_mutex1);
        std::cout << "Reading " << chunk_size << " records through connection: " << conn_index << ", from rate_id: " << first_rate_id << " to rate_id: " << last_rate_id << std::endl;
      }
      std::string query;
      query =  "select rate_table_id, code, rate, inter_rate, intra_rate, local_rate, ";
      query += "extract(epoch from effective_date) as effective_date, ";
      query += "extract(epoch from end_date) as end_date, ";
      query += "code.name as code_name, ";
      //query += "code_name, ";
      query += "resource.resource_id as egress_trunk_id ";
      query += "from rate ";
      query += "join code using (code) ";
      query += "join resource using (rate_table_id) ";
      query += "where resource.active=true and resource.egress=true";
      query += " and rate_id between " + transaction.quote(first_rate_id);
      query += " and " + transaction.quote(last_rate_id);
      query += " order by rate_id";
      pqxx::result result = transaction.exec(query);
      transaction.commit();
      {
        tbb::mutex::scoped_lock lock(track_output_mutex2);
        std::cout << "Inserting results from connection " << conn_index << " into memory structure... " << std::endl;
      }
      consolidate_results(result);
      remaining_retries = -1;
    } catch (std::exception &e) {
      remaining_retries--;
      tbb::mutex::scoped_lock lock(track_output_mutex2);
      std::cout << e.what() << std::endl;
      std::cout << "Failed query at connection: " << conn_index << ". Retrying..." << std::endl;
    }
  }
  if (remaining_retries == 0) {
    throw DBQueryFailedException();
  }
}

void DB::consolidate_results(const pqxx::result &result) {
  for (pqxx::result::const_iterator row = result.begin(); row != result.end(); ++row) {
    db_data_t db_data;
    std::string code_field_text = row[1].as<std::string>();
    code_field_text.erase(std::remove(code_field_text.begin(), code_field_text.end(), ' '), code_field_text.end()); //Cleaning database mess
    if (code_field_text == "#VALUE!")  //Ignore database mess
      continue;
    try {
      db_data.code = std::stoull(code_field_text);
    }
    catch (std::exception &e) {
      std::cerr << "BAD code: " << code_field_text << std::endl;
      continue;
    }
    if (db_data.code == 0 || std::to_string(db_data.code) != code_field_text) {
      std::cerr << "BAD code: " << code_field_text << std::endl;
      continue;
    }
    db_data.rate_table_id = row[0].as<unsigned int>();
    if (db_data.rate_table_id == 0)
      continue;
    if (row[8].is_null())
      continue;
    db_data.default_rate = row[2].is_null() ? -1 : row[2].as<double>();
    db_data.inter_rate = row[3].is_null() ? -1 : row[3].as<double>();
    db_data.intra_rate = row[4].is_null() ? -1 : row[4].as<double>();
    db_data.local_rate = row[5].is_null() ? -1 : row[5].as<double>();
    db_data.effective_date = row[6].is_null() ? -1 : row[6].as<time_t>();
    db_data.end_date = row[7].is_null() ? -1 : row[7].as<time_t>();
    db_data.code_name = row[8].as<std::string>();
    db_data.egress_trunk_id = row[9].as<unsigned int>();
    db_queue.push(db_data);
  }
}

void DB::insert_code_name_rate_table_rate(const search::SearchResult &search_result) {
  unsigned int conn_count = connections.size();
  size_t result_size = search_result.size();
  unsigned int row_per_conn = result_size / conn_count + 1;
  tbb::parallel_for(tbb::blocked_range<unsigned int>(0, conn_count),
    [&](const tbb::blocked_range<unsigned int>& r) {
      for (auto conn_index = r.begin(); conn_index != r.end(); ++conn_index) {
        size_t result_ini = conn_index * row_per_conn;
        size_t result_end_edge = result_ini + row_per_conn;
        if (result_end_edge > result_size)
          result_end_edge = result_size;
        for (size_t i = result_ini; i < result_end_edge; i++) {
          search::SearchResultElement* data = search_result[i];
          pqxx::work transaction( *connections[conn_index] );
          std::string query;
          query =  "insert into code_name_rate_table_rate ";
          query += "(time, code_name, rate_table_id,";
          query += " current_min_rate, current_max_rate,";
          query += " future_min_rate, future_max_rate) ";
          query += "values (";
          query +=  transaction.quote(time(nullptr)) + ", ";
          query +=  transaction.quote(data->get_code_name()) + ", ";
          query +=  transaction.quote(data->get_rate_table_id()) + ", ";
          query +=  transaction.quote(data->get_current_min_rate()) + ", ";
          query +=  transaction.quote(data->get_current_max_rate()) + ", ";
          query +=  (data->get_future_min_rate() == -1 ? "NULL" : transaction.quote(data->get_future_min_rate())) + ", ";
          query +=  (data->get_future_max_rate() == -1 ? "NULL" : transaction.quote(data->get_future_max_rate())) + ")";
          pqxx::result result = transaction.exec(query);
          transaction.commit();
        }
      }
    });
}
