#ifndef DAEMON_H
#define DAEMON_H

#include "db.hxx"
#include "trie.hxx"
#include "tbb/flow_graph.h"
#include <unordered_map>

using namespace tbb::flow;

namespace ctrl {

  class ConnectionInfo {
    public:
      std::string host;
      std::string dbname;
      std::string user;
      std::string password;
      unsigned int port;
    };

  class Controller {
    private:
      graph tbb_graph;
      std::shared_ptr<db::DB> database;
      std::shared_ptr<ConnectionInfo> conn_info;
      unsigned int telnet_listen_port;
      unsigned int http_listen_port;
      std::shared_ptr<std::unordered_map<unsigned int, std::shared_ptr<trie::Trie>>> tables_tries;
      void connect_to_database();
      void insert_new_rate_data(std::shared_ptr<db::RateRecord> new_record);
      void set_new_rate_data();
      void update_rate_data();
    public:
      Controller(std::shared_ptr<ConnectionInfo> conn_info,
                 unsigned int telnet_listen_port,
                 unsigned int http_listen_port);
      void workflow();
  };
}
#endif
