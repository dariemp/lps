#include "db.hxx"
#include "exceptions.hxx"
#include "controller.hxx"
#include <tbb/tbb.h>
#include <string>
#include <iostream>

using namespace db;


DB::DB(ConnectionInfo &conn_info) {
  p_conn_info = &conn_info;
  if (conn_info.conn_count < 1)
    throw DBNoConnectionsException();
  std::cout << "Connecting to database with " << conn_info.conn_count << " connections... ";
  parallel_for(tbb::blocked_range<size_t>(0, conn_info.conn_count, 1),
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
    query =  "select rate_table_id, code, rate_type, rate, inter_rate, intra_rate, local_rate, ";
    query += "extract(epoch from effective_date) as effective_date, ";
    query += "extract(epoch from end_date) as end_date, ";
    query += "code.name as code_name ";
    query += "from rate ";
    query += "join code using (code) ";
    query += "join resource using (rate_table_id) ";
    query += "where resource.active=true and resource.egress=true";
    query += " and rate_id >= " + transaction.quote(first_rate_id);
    query += " and rate_id <= " + transaction.quote(last_rate_id);
    query += " order by rate_id";
    pqxx::result result = transaction.exec(query);
    transaction.commit();
    consolidate_results(result);
  }
  else {
    std::cout << "in parallel... " << std::endl;
    std::cout << "DB rate first_rate_id: " << first_rate_id << std::endl;
    std::cout << "DB rate last_rate_id: " << last_rate_id << std::endl;
    unsigned int total_row_count = last_rate_id - first_rate_id;
    unsigned int iter_row_count = conn_count * 100000;
    unsigned int iter_count = total_row_count / iter_row_count + 1;
    for (unsigned int i = 0; i < iter_count; i++)
      parallel_for(tbb::blocked_range<unsigned int>(0, conn_count, 1),
        [=](const tbb::blocked_range<unsigned int>& r) {
              unsigned int conn_index = r.begin();
              if (conn_index >= conn_count)
                conn_index = conn_count - 1;
              unsigned int range_first_rate_id =  first_rate_id + i * iter_row_count + conn_index * 100000;
              unsigned int range_last_rate_id =  first_rate_id + i * iter_row_count + (conn_index + 1) * 100000 - 1;
              pqxx::work transaction( *connections[conn_index].get() );
              track_output_mutex1.lock();
              std::cout << "Reading 100000 records through connection: " << conn_index << ", from rate_id: " << range_first_rate_id << " to rate_id: " << range_last_rate_id << std::endl;
              track_output_mutex1.unlock();
              std::string query;
              query =  "select rate_table_id, code, rate_type, rate, inter_rate, intra_rate, local_rate, ";
              query += "extract(epoch from effective_date) as effective_date, ";
              query += "extract(epoch from end_date) as end_date, ";
              query += "code.name as code_name ";
              query += "from rate ";
              query += "join code using (code) ";
              query += "join resource using (rate_table_id) ";
              query += "where resource.active=true and resource.egress=true";
              query += " and rate_id >= " + transaction.quote(range_first_rate_id);
              query += " and rate_id <= " + transaction.quote(range_last_rate_id);
              query += " order by rate_id";
              pqxx::result result = transaction.exec(query);
              transaction.commit();
              track_output_mutex2.lock();
              std::cout << "Inserting results from connection " << conn_index << " into memory structure... " << std::endl;
              track_output_mutex2.unlock();
              consolidate_results(result);
        });
  }
  std::cout << "done." << std::endl;
}

void DB::consolidate_results(pqxx::result result) {
  for (pqxx::result::const_iterator row = result.begin(); row != result.end(); ++row) {
    double selected_rate;
    try {
      int rate_type = row[2].as<int>();
      switch (rate_type) {
        case 1:
          selected_rate = row[4].as<double>();
          break;
        case 2:
          selected_rate = row[5].as<double>();
          break;
        case 3:
          selected_rate = row[6].as<double>();
          break;
        default:
          selected_rate = row[3].as<double>();
      }
    } catch (std::exception &e) {
      continue;
    }
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
    time_t effective_date = row[7].is_null() ? -1 : row[7].as<time_t>();
    time_t end_date = row[8].is_null() ? -1 : row[8].as<time_t>();
    ctrl::p_controller_t controller = ctrl::Controller::get_controller();
    controller->insert_new_rate_data(rate_table_id, code, selected_rate, effective_date, end_date);
  }
}
