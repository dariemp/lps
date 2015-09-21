#include "db.hxx"
#include "exceptions.hxx"
#include "controller.hxx"
#include <tbb/tbb.h>
#include <string>
#include <iostream>
using namespace db;

RateRecord::RateRecord(unsigned int rate_table_id, std::string prefix, double rate, time_t effective_date, time_t end_date)
  : rate_table_id(rate_table_id), prefix(prefix), rate(rate), effective_date(effective_date), end_date(end_date) {}

unsigned int RateRecord::get_rate_table_id() {
  return this->rate_table_id;
}
std::string RateRecord::get_prefix() {
  return prefix;
}

double RateRecord::get_rate() {
  return rate;
}

time_t RateRecord::get_effective_date() {
  return effective_date;
}

time_t RateRecord::get_end_date() {
  return end_date;
}

DB::DB(std::string host, std::string dbname, std::string user, std::string password, unsigned int port, unsigned int conn_count) {
  if (conn_count < 1)
    throw DBNoConnectionsException();
  last_rate_id = 0;
  std::cout << "Connecting to database with " << conn_count << " connections... ";
  connections = uptr_connections_t(new connections_t());
  parallel_for(tbb::blocked_range<size_t>(0, conn_count, 1),
      [=](const tbb::blocked_range<size_t>& r) {
          for (size_t i = r.begin(); i != r.end(); ++i) {
            pqxx::connection* conn = new pqxx::connection("host=" + host + " dbname=" + dbname + " user=" + user + " password=" + password + " port=" + std::to_string(port));
            add_conn_mutex.lock();
            connections->emplace_back(conn);
            add_conn_mutex.unlock();
          }
      });
  std::cout << "done." << std::endl;
}

unsigned int DB::get_first_rate_id(bool from_beginning) {
  unsigned int start_rate_id = last_rate_id + 1;
  std::unique_ptr<pqxx::work> transaction = std::unique_ptr<pqxx::work>(new pqxx::work( *(*connections)[0].get() ));
  std::string order = from_beginning ? "" : "desc ";
  pqxx::result result = transaction->exec("select rate_id from rate where rate_id >= " + transaction->quote(start_rate_id) + " order by rate_id " + order + "limit 1");
  transaction->commit();
  if (result.empty())
    return 0;
  else
    return result.begin()["rate_id"].as<unsigned int>();
}

void DB::get_new_records() {
  if (last_rate_id == 0)
    std::cout << "Loading rate records from the database for the first time (takes some time)... ";
  else
    std::cout << "Loading new rate records from the database... ";
  unsigned int first_rate_id = get_first_rate_id();
  last_rate_id = get_first_rate_id(false);
  unsigned int conn_count = connections->size();
  if (conn_count == 1) {
    std::unique_ptr<pqxx::work> transaction = std::unique_ptr<pqxx::work>(new pqxx::work( *(*connections)[0].get() ));
    pqxx::result result = transaction->exec("select rate_table_id, code, rate_type, rate, inter_rate, intra_rate, local_rate, extract(epoch from effective_date) as effective_date, extract(epoch from end_date) as end_date from rate where rate_id >= " + transaction->quote(first_rate_id) + " order by rate_id");
    transaction->commit();
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
              std::unique_ptr<pqxx::work> transaction = std::unique_ptr<pqxx::work>(new pqxx::work( *(*connections)[conn_index].get() ));
              track_output_mutex.lock();
              std::cout << "Reading 100000 records through connection: " << conn_index << ", from rate_id: " << range_first_rate_id << std::endl;
              track_output_mutex.unlock();
              pqxx::result result = transaction->exec("select rate_table_id, code, rate_type, rate, inter_rate, intra_rate, local_rate, extract(epoch from effective_date) as effective_date, extract(epoch from end_date) as end_date from rate where rate_id >= " + transaction->quote(range_first_rate_id) + " order by rate_id limit 100000");
              transaction->commit();
              std::cout << "Inserting results into memory structure... " << std::endl;
              consolidate_results(result);
        });
  }
  std::cout << "done." << std::endl;
}

void DB::consolidate_results(pqxx::result result) {
  tbb::mutex::scoped_lock lock(consolidation_mutex);
  pqxx::result::const_iterator row;
  for (row = result.begin(); row != result.end(); row++) {
    double selected_rate;
    try {
      int rate_type = row["rate_type"].as<int>();
      switch (rate_type) {
        case 1:
          selected_rate = row["inter_rate"].as<double>();
          break;
        case 2:
          selected_rate = row["intra_rate"].as<double>();
          break;
        case 3:
          selected_rate = row["local_rate"].as<double>();
          break;
        default:
          selected_rate = row["rate"].as<double>();
      }
    } catch (std::exception &e) {
      continue;
    }
    std::string prefix = row["code"].as<std::string>();
    prefix.erase(std::remove(prefix.begin(), prefix.end(), ' '), prefix.end()); //Cleaning database mess
    if (prefix == "#VALUE!")  //Cleaning database mess
      continue;
    try {
      std::strtoull(prefix.c_str(), nullptr, 10);
    } catch (std::exception &e) {
      continue;
    }
    ctrl::p_controller_t controller = ctrl::Controller::get_controller();
    controller->insert_new_rate_data(new RateRecord(
      row["rate_table_id"].as<unsigned int>(),
      prefix,
      selected_rate,
      row["effective_date"].is_null() ? -1 : row["effective_date"].as<time_t>(),
      row["end_date"].is_null() ? -1 : row["end_date"].as<time_t>()));
  }
}
