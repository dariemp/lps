#include "db.hxx"
#include <string>
#include <iostream>
using namespace db;

RateRecord::RateRecord(unsigned int rate_table_id, unsigned int prefix, double rate, time_t effective_date, time_t end_date)
  : rate_table_id(rate_table_id), prefix(prefix), rate(rate), effective_date(effective_date), end_date(end_date) {}

unsigned int RateRecord::get_rate_table_id() {
  return this->rate_table_id;
}
unsigned int RateRecord::get_prefix() {
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

DB::DB(std::string host, std::string dbname, std::string user, std::string password, unsigned int port) {
  last_rate_id = 0;
  connection = std::unique_ptr<pqxx::connection>(new pqxx::connection("host=" + host + " dbname=" + dbname + " user=" + user + " password=" + password + " port=" + std::to_string(port)));
}

p_rate_records_t DB::get_new_records() {
  std::unique_ptr<pqxx::work> transaction = std::unique_ptr<pqxx::work>(new pqxx::work(*connection));
  unsigned int start_rate_id = last_rate_id + 1;
  if (start_rate_id == 1)
    std::cout << "Loading rate records from the database for the first time (takes some time)... ";
  else
    std::cout << "Loading new rate records from the database...";
  pqxx::result result = transaction->exec("select rate_id, rate_table_id, code, rate_type, rate, inter_rate, intra_rate, local_rate, extract(epoch from effective_date) as effective_date, extract(epoch from end_date) as end_date from rate where rate_id > " + transaction->quote(start_rate_id) + " order by rate_id limit 1000");
  transaction->commit();
  std::cout << "done." << std::endl;
  std::cout << "Processing loaded records...";
  p_rate_records_t rate_records = new rate_records_t();
  pqxx::result::const_iterator row;
  for (row = result.begin(); row != result.end(); row++) {
    int rate_type = row["rate_type"].as<int>();
    double selected_rate;
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
    rate_records->emplace_back(new RateRecord(
      row["rate_table_id"].as<unsigned int>(),
      row["code"].as<unsigned int>(),
      selected_rate,
      row["effective_date"].is_null() ? -1 : row["effective_date"].as<time_t>(),
      row["end_date"].is_null() ? -1 : row["end_date"].as<time_t>()));
  }
  last_rate_id = (--row)["rate_id"].as<unsigned int>();
  std::cout << "done." << std::endl;
  return rate_records;
}
