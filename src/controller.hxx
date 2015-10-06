#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "db.hxx"
#include "trie.hxx"
#include "search_result.hxx"
#include "shared_types.hxx"
#include <unordered_map>
#include <memory>
#include <vector>
#include <atomic>
#include <condition_variable>

namespace ctrl {

  class Controller;
  typedef std::vector<unsigned int> rate_table_ids_t;
  typedef std::unique_ptr<rate_table_ids_t> uptr_rate_table_ids_t;
  typedef std::unordered_map<unsigned int, trie::uptr_trie_t> table_tries_map_t;
  typedef std::unique_ptr<table_tries_map_t> uptr_table_tries_map_t;
  typedef std::unique_ptr<Controller> uptr_controller_t;
  typedef Controller* p_controller_t;
  static tbb::mutex map_insertion_mutex;
  static tbb::mutex code_insertion_mutex;
  static std::mutex update_tables_mutex;
  static std::condition_variable condition_variable_tables;
  static std::atomic_bool updating_tables;
  //static tbb::mutex output_mutex;

  class Controller {
    private:
      db::uptr_db_t database;
      db::p_conn_info_t conn_info;
      uptr_rate_table_ids_t rate_table_ids;
      uptr_table_tries_map_t tables_tries;
      uptr_code_names_t code_names;
      uptr_codes_t codes;
      uptr_rate_table_ids_t new_rate_table_ids;
      uptr_table_tries_map_t new_tables_tries;
      uptr_code_names_t new_code_names;
      uptr_codes_t new_codes;
      unsigned int telnet_listen_port;
      unsigned int http_listen_port;
      void reset_new_tables();
      void update_rate_tables_tries();
      void create_table_tries();
      void update_table_tries();
      void run_http_server();
      void run_telnet_server();
      void _search_code(std::string code, search::SearchResult &result);
      Controller(db::ConnectionInfo &conn_info,
                 unsigned int telnet_listen_port,
                 unsigned int http_listen_port);
    public:
      static p_controller_t get_controller(db::ConnectionInfo &conn_info,
                                        unsigned int telnet_listen_port,
                                        unsigned int http_listen_port);
      static p_controller_t get_controller();
      void insert_new_code(unsigned long long code, std::string code_name);
      void insert_new_rate_data(unsigned int rate_table_id,
                                std::string code,
                                double rate,
                                time_t effective_date,
                                time_t end_date);
      void start_workflow();
      void search_code(std::string code, search::SearchResult &result);
      void search_code_name(std::string code_name, search::SearchResult &result);
      void search_code_name_rate_table(std::string code_name, unsigned int rate_table_id, search::SearchResult &result);
      void search_rate_table(unsigned int rate_table_id, search::SearchResult &result);
      void search_all_codes(search::SearchResult &result);
  };
}
#endif
