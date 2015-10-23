#ifndef DB_H
#define DB_H

#include <tbb/tbb.h>
#include <pqxx/pqxx>
#include "search_result.hxx"

namespace db {

  class ConnectionInfo;
  class DB;
  typedef ConnectionInfo* p_conn_info_t;
  typedef DB* p_db_t;
  typedef pqxx::connection* p_connection_t;
  typedef tbb::concurrent_vector<p_connection_t> connections_t;

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
      unsigned int chunk_size;
  };

  class DB {
    private:
      tbb::mutex track_output_mutex1;
      tbb::mutex track_output_mutex2;
      p_conn_info_t p_conn_info;
      connections_t connections;
      unsigned int get_first_rate_id(bool from_beginning=true);
      void query_database(unsigned int conn_index, const std::string chunk_size, int first_rate_id, unsigned int last_rate_id);
      void consolidate_results(const pqxx::result &result);
    public:
      DB(ConnectionInfo &conn_info);
      ~DB();
      unsigned int get_refresh_minutes();
      void get_new_records();
      void insert_code_name_rate_table_rate(const search::SearchResult &search_result);
  };
}
#endif
