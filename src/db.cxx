#include "db.hxx"
#include <string>
using namespace db;

RateRecords::RateRecords(unsigned int rate_table_id, unsigned int prefix, double rate, time_t effective_date, time_t end_date)
  : rate_table_id(rate_table_id), prefix(prefix), rate(rate), effective_date(effective_date), end_date(end_date) {}

unsigned int RateRecords::get_rate_table_id() {
  return rate_table_id;
}
unsigned int RateRecords::get_prefix() {
  return prefix;
}

double RateRecords::get_rate() {
  return rate;
}

time_t RateRecords::get_effective_date() {
  return effective_date;
}

time_t RateRecords::get_end_date() {
  return end_date;
}

DB::DB(std::string host, std::string dbname, std::string user, std::string password, unsigned int port) {
  last_rate_id = 0;
  connection = std::make_shared<pqxx::connection>("host=" + host + " database=" + dbname + " user=" + user + " password=" + password + " port=" + std::to_string(port));
}

std::shared_ptr<std::vector<std::shared_ptr<RateRecords>>> DB::get_new_records() {
  std::shared_ptr<pqxx::work> transaction = std::make_shared<pqxx::work>(*connection);
  unsigned int start_rate_id = last_rate_id + 1;
  pqxx::result result = transaction->exec("select rate_id, rate_table_id, code, rate, effective_date, end_date from rate where rate_id > " + transaction->quote(start_rate_id) + " order by rate_id limit 20;");
  transaction->commit();
  std::shared_ptr<std::vector<std::shared_ptr<RateRecords>>> rate_records = std::make_shared<std::vector<std::shared_ptr<RateRecords>>>(result.size());
  pqxx::result::const_iterator row;
  for (row = result.begin(); row != result.end(); row++) {
    rate_records->push_back(std::make_shared<RateRecords>(
      row["rate_table_id"].as<unsigned int>(),
      row["code"].as<unsigned int>(),
      row["rate"].as<double>(),
      row["effective_date"].as<time_t>() || -1,
      row["end_date"].as<time_t>() || -1));
  }
  last_rate_id = row["rate_id"].as<unsigned int>();
  return rate_records;
}
