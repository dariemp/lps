#ifndef DB_H
#define DB_H

#include <tbb/tbb.h>
#include <pqxx/pqxx>
#include <memory>
#include <vector>
#include "search_result.hxx"

namespace db {

  class ConnectionInfo;
  class DB;
  typedef std::unique_ptr<ConnectionInfo> uptr_conn_info_t;
  typedef ConnectionInfo* p_conn_info_t;
  typedef std::unique_ptr<DB> uptr_db_t;
  typedef std::unique_ptr<pqxx::connection> uptr_connection_t;
  typedef  std::vector<uptr_connection_t> connections_t;
  static tbb::mutex add_conn_mutex;
  static tbb::mutex track_output_mutex1;
  static tbb::mutex track_output_mutex2;

  class ConnectionInfo {
    public:
      std::string host;
      std::string dbname;
      std::string user;
      std::string password;
      unsigned int port;
      unsigned int conn_count;
      unsigned int rows_to_read_debug;
      unsigned int refresh_minutes;
  };

  class DB {
    private:
      p_conn_info_t p_conn_info;
      connections_t connections;
      unsigned int get_first_rate_id(bool from_beginning=true);
      void consolidate_results(pqxx::result result);
    public:
      unsigned int get_refresh_minutes();
      DB(ConnectionInfo &conn_info);
      void get_new_records();
      void insert_code_name_rate_table_rate(const search::SearchResult &search_result);
  };
}
#endif
