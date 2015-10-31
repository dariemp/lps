#ifndef TELNET_HXX
#define TELNET_HXX

#include "telnet_resources.hxx"
#include <sys/epoll.h>
#include <libtelnet.h>
#include <string>
#include <unordered_map>
#include <queue>
#include <utility>

namespace telnet {


  class Telnet {
    private:
      typedef std::unordered_map<int, telnet_t*> telnet_ctxs_t;
      typedef telnet_ctxs_t* p_telnet_ctxs_t;
      typedef std::queue<std::string*> output_queue_t;
      typedef output_queue_t* p_output_queue_t;
      typedef std::unordered_map<int, p_output_queue_t> outputs_t;
      typedef outputs_t* p_outputs_t;
      typedef std::unordered_map<std::string, TelnetResource*> telnet_resources_t;
      typedef telnet_resources_t* p_telnet_resources_t;
      int epollfd;
      p_telnet_ctxs_t telnet_ctxs;
      p_outputs_t outputs;
      telnet_t* configure_telnet(int socket_fd);
      p_telnet_resources_t telnet_resources;
      void process_event(struct epoll_event event);
      void process_read(int socket_fd);
      void process_write(int socket_fd);
      void process_command(telnet_t *telnet , const char *buffer, size_t buffer_size);
    public:
      Telnet();
      ~Telnet();
      void register_resource(const std::string cmd, TelnetResource &resource);
      void telnet_event_handler(telnet_t *telnet, telnet_event_t *event, void *user_data);
      void run_server(unsigned int telnet_listen_port);
  };
}
#endif
