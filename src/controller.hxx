#ifndef CONTROLLER_HXX
#define CONTROLLER_HXX

#include "db.hxx"
#include "trie.hxx"
#include "search_result.hxx"
#include "shared.hxx"
#include <vector>
#include <atomic>
#include <condition_variable>

namespace ctrl {

  class Controller;
  typedef Controller* p_controller_t;


  class Controller {
    private:
      typedef std::unique_lock<std::mutex> mutex_unique_lock_t;
      typedef tbb::concurrent_queue<trie::p_trie_t> trie_release_queue_t;
      typedef tbb::concurrent_queue<p_code_list_t> code_release_queue_t;
      typedef trie_release_queue_t* p_trie_release_queue_t;
      typedef code_release_queue_t* p_code_release_queue_t;
      typedef tbb::concurrent_vector<trie_release_queue_t*> tries_release_queues_t;
      typedef tbb::concurrent_vector<p_code_release_queue_t> codes_release_queues_t;
      tries_release_queues_t tries_release_queues;
      codes_release_queues_t codes_release_queues;
      time_t reference_time;
      tbb::mutex table_insertion_mutex;
      std::mutex update_tables_mutex;
      std::mutex access_tables_mutex;
      std::condition_variable update_tables_holder;
      std::condition_variable access_tables_holder;
      std::atomic_uint table_access_count;
      db::p_db_t database;
      db::p_conn_info_t conn_info;
      trie::p_tries_t tables_tries;
      trie::p_trie_t tables_index;
      p_codes_t codes;
      trie::p_tries_t new_tables_tries;
      trie::p_trie_t new_tables_index;
      p_codes_t new_codes;
      unsigned int telnet_listen_port;
      unsigned int http_listen_port;
      std::atomic_bool updating_tables;
      std::atomic_bool clearing_tables;
      void run_worker(unsigned int worker_index);
      void run_logger();
      void run_http_server();
      void run_telnet_server();
      void reset_new_tables();
      void create_table_tries();
      void update_table_tries();
      void update_rate_tables_tries();
      void clear_tables();
      void renew_tables();
      void insert_code_name_rate_table_db();
      bool is_usa_special_case(unsigned long long code, const std::string &code_name);
      bool are_tables_available();
      void _search_code(const code_list_t &code_list, trie::rate_type_t rate_type, bool check_special_case, search::SearchResult &result);
      Controller(db::ConnectionInfo &conn_info,
                 unsigned int telnet_listen_port,
                 unsigned int http_listen_port);
      ~Controller();
    public:
      static p_controller_t get_controller(db::ConnectionInfo &conn_info,
                                        unsigned int telnet_listen_port,
                                        unsigned int http_listen_port);
      static p_controller_t get_controller();
      void start_workflow();
      void insert_new_rate_data(db::db_data_t db_data);
      void search_code(unsigned long long code, trie::rate_type_t rate_type, search::SearchResult &result);
      void search_code_name(std::string &code_name, trie::rate_type_t rate_type, search::SearchResult &result);
      void search_code_name_rate_table(std::string &code_name, unsigned int rate_table_id, trie::rate_type_t rate_type, search::SearchResult &result);
      void search_rate_table(unsigned int rate_table_id, trie::rate_type_t rate_type, search::SearchResult &result);
      void search_all_codes(trie::rate_type_t rate_type, search::SearchResult &result);
  };
}
#endif
