#ifndef DB_H
#define DB_H

#include <tbb/tbb.h>
#include <pqxx/pqxx>
#include <memory>
#include <vector>

namespace db {

  class DB;
  typedef std::unique_ptr<DB> uptr_db_t;
  typedef std::unique_ptr<pqxx::connection> uptr_connection_t;
  typedef  std::vector<uptr_connection_t> connections_t;
//  typedef std::unique_ptr<connections_t> uptr_connections_t;
  static tbb::mutex add_conn_mutex;
  static tbb::mutex track_output_mutex;
  static tbb::mutex consolidation_mutex;


  class DB {
    private:
      unsigned int last_rate_id;
      //uptr_connections_t connections;
      connections_t connections;
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
