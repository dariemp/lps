#ifndef DB_H
#define DB_H

#include <tbb/tbb.h>
#include <pqxx/pqxx>
#include <memory>
#include <vector>

namespace db {

  class RateRecord;
  class DB;
  typedef std::unique_ptr<RateRecord> uptr_rate_record_t;
  typedef RateRecord* p_rate_record_t;
  typedef std::vector<uptr_rate_record_t> rate_records_t;
  typedef std::unique_ptr<rate_records_t> uptr_rate_records_t;
  typedef rate_records_t* p_rate_records_t;
  typedef std::unique_ptr<DB> uptr_db_t;
  typedef std::unique_ptr<pqxx::connection> uptr_connection_t;
  typedef  std::vector<uptr_connection_t> connections_t;
  typedef std::unique_ptr<connections_t> uptr_connections_t;
  static tbb::mutex add_conn_mutex;
  static tbb::mutex track_output_mutex;
  static tbb::mutex consolidation_mutex;


  class RateRecord {
    private:
      unsigned int rate_table_id;
      std::string prefix;
      double rate;
      time_t effective_date;
      time_t end_date;
    public:
      RateRecord(unsigned int rate_table_id, std::string prefix, double rate, time_t effective_date, time_t end_date);
      unsigned int get_rate_table_id();
      std::string get_prefix();
      double get_rate();
      time_t get_effective_date();
      time_t get_end_date();
  };

  class DB {
    private:
      unsigned int last_rate_id;
      uptr_connections_t connections;
      unsigned int get_first_rate_id(bool from_beginning=true);
      void consolidate_results(pqxx::result result);
    public:
      DB(std::string host="127.0.0.1",
         std::string dbname="icxp",
         std::string user="class4",
         std::string password="class4",
         unsigned int port=5432,
         unsigned int conn_count=10
        );
      void get_new_records();
  };
}
#endif
