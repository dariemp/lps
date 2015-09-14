#ifndef DB_H
#define DB_H

#include <pqxx/pqxx>
#include <memory>
#include <vector>

namespace db {

  class RateRecords {
    private:
      unsigned int rate_table_id;
      unsigned int prefix;
      double rate;
      time_t effective_date;
      time_t end_date;
    public:
      RateRecords(unsigned int rate_table_id, unsigned int prefix, double rate, time_t effective_date, time_t end_date);
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
      DB(std::string host, std::string dbname, std::string user, std::string password, unsigned int port=5432);
      std::shared_ptr<std::vector<std::shared_ptr<RateRecords>>> get_new_records();
  };
}
#endif
