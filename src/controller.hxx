#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "db.hxx"
#include "trie.hxx"
#include <tbb/tbb.h>
#include <tbb/flow_graph.h>
#include <unordered_map>

using namespace tbb::flow;

namespace ctrl {

  class ConnectionInfo;
  class Controller;
  typedef std::unique_ptr<ConnectionInfo> uptr_conn_info_t;
  typedef ConnectionInfo* p_conn_info_t;
  typedef std::unordered_map<unsigned int, trie::uptr_trie_t> table_tries_map_t;
  //typedef std::unique_ptr<table_tries_map_t> uptr_table_tries_map_t;
  typedef std::unique_ptr<Controller> uptr_controller_t;
  typedef Controller* p_controller_t;
  static tbb::mutex map_insertion_mutex;

  class ConnectionInfo {
    public:
      std::string host;
      std::string dbname;
      std::string user;
      std::string password;
      unsigned int port;
      unsigned int conn_count;
    };

  class Controller {
    private:
      graph tbb_graph;
      db::uptr_db_t database;
      p_conn_info_t conn_info;
      //uptr_table_tries_map_t tables_tries;
      table_tries_map_t tables_tries;
      unsigned int telnet_listen_port;
      unsigned int http_listen_port;
      void connect_to_database();
      void set_new_rate_data();
      void update_rate_data();
      Controller(ConnectionInfo &conn_info,
                 unsigned int telnet_listen_port,
                 unsigned int http_listen_port);
    public:
      static p_controller_t get_controller(ConnectionInfo &conn_info,
                                        unsigned int telnet_listen_port,
                                        unsigned int http_listen_port);
      static p_controller_t get_controller();
      void insert_new_rate_data(unsigned int rate_table_id,
                                std::string prefix,
                                double rate,
                                time_t effective_date,
                                time_t end_date);
      void workflow();
  };
}
#endif
