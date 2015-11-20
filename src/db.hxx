#ifndef DB_HXX
#define DB_HXX

#include <tbb/tbb.h>
#include <pqxx/pqxx>
#include <atomic>
#include "search_result.hxx"

namespace db {

  class ConnectionInfo;
  class DB;
  typedef ConnectionInfo* p_conn_info_t;
  typedef DB* p_db_t;

  typedef struct {
    unsigned int conn_index;
    unsigned int rate_table_id;
    unsigned long long code;
    std::string code_name;
    double default_rate;
    double inter_rate;
    double intra_rate;
    double local_rate;
    time_t effective_date;
    time_t end_date;
    unsigned int egress_trunk_id;
  } db_data_t;
  typedef tbb::concurrent_queue<db_data_t> db_queue_t;

  class ConnectionInfo {
    public:
      std::string host;
      std::string dbname;
      std::string user;
      std::string password;
      unsigned int port;
      unsigned int conn_count;
      unsigned int first_row_to_read_debug;
      unsigned int last_row_to_read_debug;
      unsigned int refresh_minutes;
      unsigned long long chunk_size;
  };

  class DB {
    private:
      typedef pqxx::connection* p_connection_t;
      typedef tbb::concurrent_vector<p_connection_t> connections_t;
      tbb::mutex range_selection_mutex;
      std::atomic_bool reading;
      std::atomic_uint reading_count;
      time_t reference_time;
      unsigned int first_rate_id;
      unsigned int last_rate_id;
      std::atomic_uint last_queried_row;
      p_conn_info_t p_conn_info;
      connections_t connections;
      unsigned int get_first_rate_id(bool from_beginning=true);
      void query_database(unsigned int conn_index, unsigned long long chunk_size, unsigned int  first_rate_id, unsigned int last_rate_id);
      void consolidate_results(unsigned int conn_index, const pqxx::result &result);
    public:
      DB(ConnectionInfo &conn_info);
      ~DB();
      void init_load_cicle(time_t reference_time);
      void wait_for_reading();
      void wait_till_next_load_cicle();
      void read_chunk(unsigned int conn_index);
      bool is_reading();
      //void insert_code_name_rate_table_rate(const search::SearchResult &search_result);
  };
}
#endif
