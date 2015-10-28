#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "db.hxx"
#include "trie.hxx"
#include "search_result.hxx"
#include "shared.hxx"
#include <vector>
#include <atomic>
#include <condition_variable>

namespace ctrl {

  class Controller;
  typedef std::unique_lock<std::mutex> mutex_unique_lock_t;
  typedef Controller* p_controller_t;

  class Controller {
    private:
      time_t reference_time;
      tbb::mutex map_insertion_mutex;
      std::mutex update_tables_mutex;
      std::mutex access_tables_mutex;
      std::atomic_bool updating_tables;
      std::atomic_uint table_access_count;
      std::condition_variable update_tables_holder;
      std::condition_variable access_tables_holder;
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
      void reset_new_tables();
      void update_rate_tables_tries();
      void create_table_tries();
      void clear_tables();
      void renew_tables();
      void update_table_tries();
      void insert_code_name_rate_table_db();
      void run_http_server();
      void run_telnet_server();
      void _search_code(const code_list_t &code_list, trie::rate_type_t rate_type, search::SearchResult &result);
      Controller(db::ConnectionInfo &conn_info,
                 unsigned int telnet_listen_port,
                 unsigned int http_listen_port);
    public:
      static p_controller_t get_controller(db::ConnectionInfo &conn_info,
                                        unsigned int telnet_listen_port,
                                        unsigned int http_listen_port);
      static p_controller_t get_controller();
      void start_workflow();
      /*void insert_new_rate_data(unsigned int rate_table_id,
                                unsigned long long code,
                                const std::string &code_name,
                                double default_rate,
                                double inter_rate,
                                double intra_rate,
                                double local_rate,
                                time_t effective_date,
                                time_t end_date,
                                unsigned int egress_trunk_id);*/
      void insert_new_rate_data(db::db_data_t db_data);
      void search_code(unsigned long long code, trie::rate_type_t rate_type, search::SearchResult &result);
      void search_code_name(const std::string &code_name, trie::rate_type_t rate_type, search::SearchResult &result);
      void search_code_name_rate_table(const std::string &code_name, unsigned int rate_table_id, trie::rate_type_t rate_type, search::SearchResult &result);
      void search_rate_table(unsigned int rate_table_id, trie::rate_type_t rate_type, search::SearchResult &result);
      void search_all_codes(trie::rate_type_t rate_type, search::SearchResult &result);
  };
}
#endif
