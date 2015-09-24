#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "db.hxx"
#include "trie.hxx"
#include "search_result.hxx"
#include <unordered_map>
#include <memory>
#include <vector>

namespace ctrl {

  class Controller;
  typedef std::vector<unsigned int> rate_table_ids_t;
  typedef std::unordered_map<unsigned int, trie::uptr_trie_t> table_tries_map_t;
  typedef std::unique_ptr<Controller> uptr_controller_t;
  typedef Controller* p_controller_t;
  static tbb::mutex map_insertion_mutex;
  //static tbb::mutex output_mutex;

  class Controller {
    private:
      db::uptr_db_t database;
      db::p_conn_info_t conn_info;
      rate_table_ids_t rate_table_ids;
      table_tries_map_t tables_tries;
      unsigned int telnet_listen_port;
      unsigned int http_listen_port;
      void create_table_tries();
      void update_table_tries();
      void run_http_server();
      Controller(db::ConnectionInfo &conn_info,
                 unsigned int telnet_listen_port,
                 unsigned int http_listen_port);
    public:
      static p_controller_t get_controller(db::ConnectionInfo &conn_info,
                                        unsigned int telnet_listen_port,
                                        unsigned int http_listen_port);
      static p_controller_t get_controller();
      void insert_new_rate_data(unsigned int rate_table_id,
                                std::string prefix,
                                double rate,
                                time_t effective_date,
                                time_t end_date);
      void start_workflow();
      void search_prefix(std::string prefix, search::SearchResult &result);
  };
}
#endif
