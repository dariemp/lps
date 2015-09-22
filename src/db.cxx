#include "db.hxx"
#include "exceptions.hxx"
#include "controller.hxx"
#include <tbb/tbb.h>
#include <string>
#include <iostream>
using namespace db;


DB::DB(std::string host, std::string dbname, std::string user, std::string password, unsigned int port, unsigned int conn_count) {
  if (conn_count < 1)
    throw DBNoConnectionsException();
  last_rate_id = 0;
  std::cout << "Connecting to database with " << conn_count << " connections... ";
  //connections = uptr_connections_t(new connections_t());
  parallel_for(tbb::blocked_range<size_t>(0, conn_count, 1),
      [=](const tbb::blocked_range<size_t>& r) {
          for (size_t i = r.begin(); i != r.end(); ++i) {
            add_conn_mutex.lock();
            connections.emplace_back(new pqxx::connection("host=" + host + " dbname=" + dbname + " user=" + user + " password=" + password + " port=" + std::to_string(port)));
            add_conn_mutex.unlock();
          }
      });
  std::cout << "done." << std::endl;
}

unsigned int DB::get_first_rate_id(bool from_beginning) {
  unsigned int start_rate_id = last_rate_id + 1;
  pqxx::work transaction(*connections[0].get());
  std::string order = from_beginning ? "" : "desc ";
  pqxx::result result = transaction.exec("select rate_id from rate where rate_id >= " + transaction.quote(start_rate_id) + " order by rate_id " + order + "limit 1");
  transaction.commit();
  if (result.empty())
    return 0;
  else
    return result.begin()[0].as<unsigned int>();
}

void DB::get_new_records() {
  if (last_rate_id == 0)
    std::cout << "Loading rate records from the database for the first time (takes some time)... ";
  else
    std::cout << "Loading new rate records from the database... ";
  unsigned int first_rate_id = get_first_rate_id();
  last_rate_id = get_first_rate_id(false);
  unsigned int conn_count = connections.size();
  if (conn_count == 1) {
    pqxx::work transaction( *connections[0].get() );
    pqxx::result result = transaction.exec("select rate_table_id, code, rate_type, rate, inter_rate, intra_rate, local_rate, extract(epoch from effective_date) as effective_date, extract(epoch from end_date) as end_date from rate where rate_id >= " + transaction.quote(first_rate_id) + " order by rate_id");
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
              track_output_mutex.lock();
              std::cout << "Reading 100000 records through connection: " << conn_index << ", from rate_id: " << range_first_rate_id << " to rate_id: " << range_last_rate_id << std::endl;
              track_output_mutex.unlock();
              pqxx::result result = transaction.exec("select rate_table_id, code, rate_type, rate, inter_rate, intra_rate, local_rate, extract(epoch from effective_date) as effective_date, extract(epoch from end_date) as end_date from rate where rate_id >= " + transaction.quote(range_first_rate_id) + " and rate_id <= " + transaction.quote(range_last_rate_id) + " order by rate_id");
              transaction.commit();
              track_output_mutex.lock();
              std::cout << "Inserting results from connection " << conn_index << " into memory structure... " << std::endl;
              track_output_mutex.unlock();
              consolidate_results(result);
        });
  }
  std::cout << "done." << std::endl;
}

void DB::consolidate_results(pqxx::result result) {
  tbb::mutex::scoped_lock lock(consolidation_mutex);
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
    std::string prefix = row[1].as<std::string>();
    prefix.erase(std::remove(prefix.begin(), prefix.end(), ' '), prefix.end()); //Cleaning database mess
    if (prefix == "#VALUE!")  //Ignore database mess
      continue;
    unsigned long long numeric_prefix = std::strtoull(prefix.c_str(), nullptr, 10);
    if (numeric_prefix == 0 || std::to_string(numeric_prefix) != prefix) {
      std::cerr << "BAD PREFIX: " << prefix << std::endl;
      continue;
    }
    unsigned int rate_table_id = row[0].as<unsigned int>();
    time_t effective_date = row[7].is_null() ? -1 : row[7].as<time_t>();
    time_t end_date = row[8].is_null() ? -1 : row[8].as<time_t>();
    ctrl::p_controller_t controller = ctrl::Controller::get_controller();
    controller->insert_new_rate_data(rate_table_id, prefix, selected_rate, effective_date, end_date);
  }
}
