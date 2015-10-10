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
  parallel_for(tbb::blocked_range<size_t>(0, conn_info.conn_count),
      [=](const tbb::blocked_range<size_t>& r) {
          for (size_t i = r.begin(); i != r.end(); ++i) {
            add_conn_mutex.lock();
            connections.emplace_back(new pqxx::connection("host=" + conn_info.host + " dbname=" + conn_info.dbname + " user=" + conn_info.user + " password=" + conn_info.password + " port=" + std::to_string(conn_info.port)));
            add_conn_mutex.unlock();
          }
      });
  std::cout << "done." << std::endl;
}

unsigned int DB::get_refresh_minutes() {
  return p_conn_info->refresh_minutes;
}

unsigned int DB::get_first_rate_id(bool from_beginning) {
  if (!from_beginning && p_conn_info->rows_to_read_debug)
    return p_conn_info->rows_to_read_debug;
  else {
    pqxx::work transaction(*connections[0].get());
    std::string order = from_beginning ? "" : "desc ";
    pqxx::result result = transaction.exec("select rate_id from rate order by rate_id " + order + "limit 1");
    transaction.commit();
    if (result.empty())
      return 0;
    else
      return result.begin()[0].as<unsigned int>();
  }
}

void DB::get_new_records() {
  std::cout << "Loading rate records from the database... ";
  unsigned int first_rate_id = get_first_rate_id();
  unsigned int last_rate_id = get_first_rate_id(false);
  unsigned int conn_count = connections.size();
  if (conn_count == 1) {
    pqxx::work transaction( *connections[0].get() );
    std::string query;
    query =  "select rate_table_id, code, rate, inter_rate, intra_rate, local_rate, ";
    query += "extract(epoch from effective_date) as effective_date, ";
    query += "extract(epoch from end_date) as end_date, ";
    query += "code.name as code_name ";
    query += "from rate ";
    query += "join code using (code) ";
    query += "join resource using (rate_table_id) ";
    query += "where resource.active=true and resource.egress=true";
    query += " and rate_id between " + transaction.quote(first_rate_id);
    query += " and " + transaction.quote(last_rate_id);
    query += " order by rate_id";
    pqxx::result result = transaction.exec(query);
    transaction.commit();
    consolidate_results(result);
  }
  else {
    std::cout << "in parallel... " << std::endl;
    std::cout << "DB rate first_rate_id: " << first_rate_id << std::endl;
    std::cout << "DB rate last_rate_id: " << last_rate_id << std::endl;
    unsigned int chunk_size = p_conn_info->chunk_size;
    std::atomic_uint last_queried_row;
    last_queried_row = first_rate_id - 1;
    tbb::mutex range_selection_mutex;
    parallel_for (tbb::blocked_range<size_t>(0, conn_count),
      [&](const tbb::blocked_range<size_t> &r){
        tbb::task_group tasks;
        for (size_t conn_index = r.begin(); conn_index != r.end(); ++conn_index) {
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
            int remaining_retries = 3;
            while (remaining_retries > 0) {
              try {
                pqxx::work transaction(*connections[conn_index].get());
                {
                  tbb::mutex::scoped_lock lock(track_output_mutex1);
                  std::cout << "Reading " << chunk_size << " records through connection: " << conn_index << ", from rate_id: " << range_first_rate_id << " to rate_id: " << range_last_rate_id << std::endl;
                }
                std::string query;
                query =  "select rate_table_id, code, rate, inter_rate, intra_rate, local_rate, ";
                query += "extract(epoch from effective_date) as effective_date, ";
                query += "extract(epoch from end_date) as end_date, ";
                query += "code.name as code_name ";
                query += "from rate ";
                query += "join code using (code) ";
                query += "join resource using (rate_table_id) ";
                query += "where resource.active=true and resource.egress=true";
                query += " and rate_id between " + transaction.quote(range_first_rate_id);
                query += " and " + transaction.quote(range_last_rate_id);
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
                std::cout << "Failed query at connection: " << conn_index << ". Retrying..." << std::endl;
              }
            }
            if (remaining_retries == 0) {
              throw DBQueryFailedException();
            }
          }
        }
      });
  }
  std::cout << "done." << std::endl;
}

void DB::consolidate_results(pqxx::result result) {
  for (pqxx::result::const_iterator row = result.begin(); row != result.end(); ++row) {
    std::string code = row[1].as<std::string>();
    code.erase(std::remove(code.begin(), code.end(), ' '), code.end()); //Cleaning database mess
    if (code == "#VALUE!")  //Ignore database mess
      continue;
    unsigned long long numeric_code = std::strtoull(code.c_str(), nullptr, 10);
    if (numeric_code == 0 || std::to_string(numeric_code) != code) {
      std::cerr << "BAD code: " << code << std::endl;
      continue;
    }
    unsigned int rate_table_id = row[0].as<unsigned int>();
    double default_rate = row[2].is_null() ? -1 : row[2].as<double>();
    double inter_rate = row[3].is_null() ? -1 : row[3].as<double>();
    double intra_rate = row[4].is_null() ? -1 : row[4].as<double>();
    double local_rate = row[5].is_null() ? -1 : row[5].as<double>();
    time_t effective_date = row[6].is_null() ? -1 : row[6].as<time_t>();
    time_t end_date = row[7].is_null() ? -1 : row[7].as<time_t>();
    std::string code_name = row[8].as<std::string>();
    ctrl::p_controller_t controller = ctrl::Controller::get_controller();
    controller->insert_new_rate_data(rate_table_id, code, numeric_code, code_name, default_rate, inter_rate, intra_rate, local_rate, effective_date, end_date);
  }
}

void DB::insert_code_name_rate_table_rate(const search::SearchResult &search_result) {
  unsigned int conn_count = connections.size();
  size_t result_size = search_result.size();
  unsigned int row_per_conn = result_size / conn_count + 1;
  parallel_for(tbb::blocked_range<unsigned int>(0, conn_count),
    [&](const tbb::blocked_range<unsigned int>& r) {
      for (auto conn_index = r.begin(); conn_index != r.end(); ++conn_index) {
        size_t result_ini = conn_index * row_per_conn;
        size_t result_end_edge = result_ini + row_per_conn;
        if (result_end_edge > result_size)
          result_end_edge = result_size;
        for (size_t i = result_ini; i < result_end_edge; i++) {
          search::SearchResultElement* data = search_result[i];
          pqxx::work transaction( *connections[conn_index].get() );
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
