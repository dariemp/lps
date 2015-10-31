#include "db.hxx"
#include "exceptions.hxx"
#include "controller.hxx"
#include "logger.hxx"
#include <tbb/tbb.h>
#include <string>
#include <iostream>
#include <atomic>
#include <thread>

using namespace db;

DB::DB(ConnectionInfo &conn_info) : reading(false), reading_count(0) {
  p_conn_info = &conn_info;
  if (conn_info.conn_count < 1)
    throw DBNoConnectionsException();
  ctrl::log("Connecting to database with " + std::to_string(conn_info.conn_count) + " connections...\n");
  tbb::parallel_for(tbb::blocked_range<size_t>(0, conn_info.conn_count),
      [=](const tbb::blocked_range<size_t>& r) {
          for (size_t i = r.begin(); i != r.end(); ++i)
            connections.push_back(new pqxx::connection("host=" + conn_info.host + " dbname=" + conn_info.dbname + " user=" + conn_info.user + " password=" + conn_info.password + " port=" + std::to_string(conn_info.port)));
      });
}

DB::~DB() {
  for (size_t i = 0; i < connections.size(); ++i)
    delete connections[i];
  connections.clear();
}

bool DB::is_reading() {
  return reading;
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

void DB::init_load_cicle() {
  ctrl::log("Loading rate records from the database in parallel... \n");
  if (p_conn_info->first_row_to_read_debug)
    first_rate_id = p_conn_info->first_row_to_read_debug;
  else
    first_rate_id = get_first_rate_id();
  if (p_conn_info->last_row_to_read_debug)
    last_rate_id = p_conn_info->last_row_to_read_debug;
  else
    last_rate_id = get_first_rate_id(false);
  ctrl::log("DB rate first_rate_id: " + std::to_string(first_rate_id) + "\n");
  ctrl::log("DB rate last_rate_id: " + std::to_string(last_rate_id) + "\n");
  last_queried_row = first_rate_id - 1;
  reading = true;
  reading_count = 0;
}

void DB::read_chunk(unsigned int conn_index) {
  reading_count++;
  unsigned long long chunk_size = p_conn_info->chunk_size;
  tbb::mutex range_selection_mutex;
  unsigned int range_first_rate_id;
  unsigned int range_last_rate_id;
  {
    tbb::mutex::scoped_lock lock(range_selection_mutex);
    range_first_rate_id = last_queried_row + 1;
    range_last_rate_id = range_first_rate_id + chunk_size;
    if (range_last_rate_id > last_rate_id) {
      range_last_rate_id = last_rate_id;
      reading = false;
    }
    last_queried_row = range_last_rate_id;
  }
  query_database(conn_index, chunk_size, range_first_rate_id, range_last_rate_id);
  reading_count--;
  reading = last_queried_row < last_rate_id;
}

void DB::query_database(unsigned int conn_index, unsigned long long chunk_size, unsigned int first_rate_id, unsigned int last_rate_id) {
  int remaining_retries = 3;
  while (remaining_retries > 0) {
    try {
      pqxx::work transaction(*connections[conn_index]);
      ctrl::log("Reading " + std::to_string(chunk_size) + " records through connection: " + std::to_string(conn_index) + ", from rate_id: " + std::to_string(first_rate_id) + " to rate_id: " + std::to_string(last_rate_id) + "\n");
      std::string query;
      query =  "select rate_table_id, code, rate, inter_rate, intra_rate, local_rate, ";
      query += "extract(epoch from effective_date) as effective_date, ";
      query += "extract(epoch from end_date) as end_date, ";
      //query += "code.name as code_name, ";
      query += "code_name, ";
      query += "resource.resource_id as egress_trunk_id ";
      query += "from rate ";
      //query += "join code using (code) ";
      query += "join resource using (rate_table_id) ";
      query += "where resource.active=true and resource.egress=true";
      query += " and rate_id between " + transaction.quote(first_rate_id);
      query += " and " + transaction.quote(last_rate_id);
      query += " order by rate_id";
      pqxx::result result = transaction.exec(query);
      ctrl::log("Inserting results from connection " + std::to_string(conn_index) + " into memory structure...\n");
      consolidate_results(conn_index, result);
      remaining_retries = -1;
    } catch (std::exception &e) {
      remaining_retries--;
      ctrl::error(std::string(e.what()) + "\n");
      ctrl::error("Failed query at connection: " + std::to_string(conn_index) + ". Retrying...\n");
    }
  }
  if (remaining_retries == 0) {
    throw DBQueryFailedException();
  }
}

void DB::consolidate_results(unsigned int conn_index, const pqxx::result &result) {
  for (pqxx::result::const_iterator row = result.begin(); row != result.end(); ++row) {
    db_data_t db_data;
    db_data.conn_index = conn_index;
    std::string code_field_text = row[1].as<std::string>();
    code_field_text.erase(std::remove(code_field_text.begin(), code_field_text.end(), ' '), code_field_text.end()); //Cleaning database mess
    if (code_field_text == "#VALUE!")  //Ignore database mess
      continue;
    try {
      db_data.code = std::stoull(code_field_text);
    }
    catch (std::exception &e) {
      ctrl::error("BAD code: " + code_field_text + "\n");
      continue;
    }
    if (db_data.code == 0 || std::to_string(db_data.code) != code_field_text) {
      ctrl::error("BAD code: " + code_field_text + "\n");
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
    ctrl::p_controller_t controller = ctrl::Controller::get_controller();
    controller->insert_new_rate_data(db_data);
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

void DB::wait_for_reading() {
  while (reading || reading_count > 0)
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void DB::wait_till_next_load_cicle(time_t reference_time) {
  wait_for_reading();
  time_t seconds = (p_conn_info->refresh_minutes * 60) - (time(nullptr) - reference_time);
  if (seconds <= 0) {
    seconds = 0;
    ctrl::log("Next update will be now since it took too long to load from database.\n");
  }
  else if (seconds > 60) {
    unsigned int minutes = seconds / 60;
    ctrl::log("Next update from the database will be in roughly " + std::to_string(minutes) + (minutes > 1 ? " minutes.\n" : " minute.\n"));
  }
  else
    ctrl::log("Next update from the database will be in " + std::to_string(seconds) + " seconds.\n");
  std::this_thread::sleep_for(std::chrono::seconds(seconds));
}
