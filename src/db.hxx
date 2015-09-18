#ifndef DB_H
#define DB_H

#include <pqxx/pqxx>
#include <memory>
#include <vector>

namespace db {

  class RateRecord {
    private:
      unsigned int rate_table_id;
      unsigned int prefix;
      double rate;
      time_t effective_date;
      time_t end_date;
    public:
      RateRecord(unsigned int rate_table_id, unsigned int prefix, double rate, time_t effective_date, time_t end_date);
      unsigned int get_rate_table_id();
      unsigned int get_prefix();
      double get_rate();
      time_t get_effective_date();
      time_t get_end_date();

  };

  class DB {
    private:
      unsigned int last_rate_id;
      std::shared_ptr<pqxx::connection> connection;
    public:
      DB(std::string host="127.0.0.1", std::string dbname="icxp", std::string user="class4", std::string password="class4", unsigned int port=5432);
      std::shared_ptr<std::vector<std::shared_ptr<RateRecord>>> get_new_records();
  };
}
#endif
