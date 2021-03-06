#include "telnet.hxx"
#include "logger.hxx"
#include <netinet/in.h>
#include <sys/socket.h>
#include <iostream>
#include <fcntl.h>
#include <tbb/tbb.h>
#include <functional>

using namespace telnet;

Telnet::Telnet() {
  telnet_resources = new telnet_resources_t();
  telnet_ctxs = new telnet_ctxs_t();
  outputs = new outputs_t();
}

Telnet::~Telnet() {
  for (auto it = telnet_ctxs->begin(); it != telnet_ctxs->end(); ++it) {
    close(it->first);
    telnet_free(it->second);
  }
  telnet_ctxs->clear();
  delete telnet_ctxs;
  for (auto it = outputs->begin(); it != outputs->end(); ++it)
    delete it->second;
  delete outputs;
  for (auto it = telnet_resources->begin(); it != telnet_resources->end(); ++it)
    delete it->second;
  delete telnet_resources;
}

void Telnet::register_resource(const std::string cmd, TelnetResource &resource) {
  (*telnet_resources)[cmd] = &resource;
}

void Telnet::run_server(unsigned int telnet_listen_port) {
  TelnetSearchCode search_code;
  TelnetSearchCodeName search_code_name;
  TelnetSearchCodeNameAndRateTable search_code_name_rate_table;
  TelnetSearchRateTable search_rate_table;
  TelnetSearchAllCodes search_all_codes;
  register_resource("search_code", search_code);
  register_resource("search_code_name", search_code_name);
  register_resource("search_code_name_rate_table", search_code_name_rate_table);
  register_resource("search_rate_table", search_rate_table);
  register_resource("search_all_az_codes", search_all_codes);
  int telnet_socket, telnet_socket6;
  struct sockaddr_in addr, addr6, local;
  struct epoll_event ev;
  int conn_sock, nfds;
  int addrlen = sizeof(struct sockaddr_in);
  memset(&addr, 0, addrlen);
  memset(&addr6, 0, addrlen);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(telnet_listen_port);
  addr6.sin_family = AF_INET6;
  addr6.sin_addr.s_addr = INADDR_ANY;
  addr6.sin_port = htons(telnet_listen_port);
  telnet_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  telnet_socket6 = socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK | IPV6_V6ONLY, 0);
  epollfd = epoll_create(10);
  if (epollfd == -1) {
    ctrl::error("Could not create network queue.");
    exit(EXIT_FAILURE);
  }
  if (telnet_socket < 0 && telnet_socket6 < 0) {
    ctrl::error("Could not create telnet server socket");
    exit(EXIT_FAILURE);
  }
  if (telnet_socket >= 0 ) {
    if (bind(telnet_socket, (struct sockaddr *)&addr, addrlen) < 0) {
      ctrl::error("Could not bind telnet IPV4 socket");
      exit(EXIT_FAILURE);
    }
    if (listen(telnet_socket, 10) < 0) {
      ctrl::error("Could not listen on telnet IPV4 socket");
      exit(EXIT_FAILURE);
    };
    ev.events = EPOLLIN;
    ev.data.fd = telnet_socket;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, telnet_socket, &ev) == -1) {
       ctrl::error("Could not initialize listening queue (IPV4)");
       exit(EXIT_FAILURE);
    }
  }
  if (telnet_socket6 >= 0 ) {
    if (bind(telnet_socket6, (struct sockaddr *)&addr6, addrlen) < 0) {
      ctrl::error("Could not bind telnet IPV6 socket");
      exit(EXIT_FAILURE);
    }
    if (listen(telnet_socket6, 10) < 0) {
      ctrl::error("Could not listen on telnet IPV6 socket");
      exit(EXIT_FAILURE);
    }
    ev.events = EPOLLIN;
    ev.data.fd = telnet_socket6;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, telnet_socket6, &ev) == -1) {
       ctrl::error("Could not initialize listening queue (IPV6)");
       exit(EXIT_FAILURE);
    }
  }
  ctrl::log("Listening Telnet server at port " + std::to_string(telnet_listen_port) + "...");
  struct epoll_event events[10];
  tbb::task_group g;
  for (;;) {
    nfds = epoll_wait(epollfd, events, 10, -1);
    if (nfds == -1) {
      ctrl::error("Error when trying to capture networking events.");
      exit(EXIT_FAILURE);
    }
    for (int i = 0; i < nfds; ++i) {
      struct epoll_event event = events[i];
      int listen_sock;
      if (event.data.fd == telnet_socket)
        listen_sock = telnet_socket;
      else if (event.data.fd == telnet_socket6)
        listen_sock = telnet_socket6;
      else
        listen_sock = -1;
      if (listen_sock != -1) {
        conn_sock = accept(listen_sock, (struct sockaddr *) &local, ( socklen_t*)&addrlen);
        if (conn_sock == -1) {
          ctrl::error("Failed to accept client connection.");
          exit(EXIT_FAILURE);
        }
        int fdflags;
        if ((fdflags = fcntl(conn_sock, F_GETFL, 0)) < 0) {
          ctrl::error("Failed to obtain connection information.");
          exit(EXIT_FAILURE);
        }
        fdflags |= O_NONBLOCK;
        if (fcntl(conn_sock, F_SETFL, fdflags) == -1) {
          ctrl::error("Failed to set connection settings.");
          exit(EXIT_FAILURE);
        }
        telnet_t* telnet_ctx = configure_telnet(conn_sock);
        if (!telnet_ctx) {
          ctrl::error("Could not start telnet protocol.");
          exit(EXIT_FAILURE);
        }
        (*telnet_ctxs)[conn_sock] = telnet_ctx;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = conn_sock;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1) {
           ctrl::error("Failed to queue new connection.");
           exit(EXIT_FAILURE);
        }
      }
      else g.run([&]{ process_event(event); } );
    }
  }
}

void Telnet::process_event(struct epoll_event event) {
  if (event.events & EPOLLIN) {
    process_read(event.data.fd);
  }
  if (event.events & EPOLLOUT) {
    process_write(event.data.fd);
  }
}

void Telnet::process_read(int socket_fd) {
  std::string data_read;
  char buffer[128];
  int bytes_read;
  while ((bytes_read = read(socket_fd, &buffer, 128)) > 0) {
    buffer[bytes_read-1] = 0;
    data_read += buffer;
  }
  telnet_recv((*telnet_ctxs)[socket_fd], data_read.c_str(), data_read.size());
}

void Telnet::process_write(int socket_fd) {
  output_queue_t* output_queue = (*outputs)[socket_fd];
  while (!output_queue->empty()) {
    std::string* output = output_queue->front();
    int bytes_written, bytes_to_send = output->size();
    const char* data = output->c_str();
    while ((bytes_written = send(socket_fd, data, bytes_to_send, 0)) < bytes_to_send) {
      bytes_to_send -= bytes_written;
      data += bytes_written;
    }
    output_queue->pop();
    delete output;
  }
  epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = socket_fd;
  if (epoll_ctl(epollfd, EPOLL_CTL_MOD, socket_fd, &ev) == -1) {
     ctrl::error("Failed to change socket direction.");
     exit(EXIT_FAILURE);
  }
}

typedef struct {
  int socket_fd;
  Telnet* server;
} user_data_t;

void Telnet::telnet_event_handler(telnet_t *telnet, telnet_event_t *event, void *user_data) {
  user_data_t* data = (user_data_t*)user_data;
  int socket_fd = data->socket_fd;
  p_output_queue_t output_queue;
  std::string* output;
  switch (event->type) {
  	/* data received */
  	case TELNET_EV_DATA:
      (*outputs)[socket_fd] = new output_queue_t();
  		process_command(telnet, event->data.buffer, event->data.size);
  		break;
  	/* data must be sent */
  	case TELNET_EV_SEND:
      (*outputs)[socket_fd]->emplace(new std::string(event->data.buffer, event->data.size));
      if (event->data.size == 2 && event->data.buffer[0] == '\xff' && event->data.buffer[1] == '\xf9') {
        epoll_event ev;
        ev.events = EPOLLOUT | EPOLLET;
        ev.data.fd = socket_fd;
        if (epoll_ctl(epollfd, EPOLL_CTL_MOD, socket_fd, &ev) == -1) {
          ctrl::error("Failed to change socket direction.");
          exit(EXIT_FAILURE);
        }
      }
  		break;
  	/* enable compress2 if accepted by client */
  	case TELNET_EV_DO:
  		if (event->neg.telopt == TELNET_TELOPT_COMPRESS2)
  			telnet_begin_compress2(telnet);
  		break;
  	/* error */
  	case TELNET_EV_ERROR:
  		close(socket_fd);
      telnet_free((*telnet_ctxs)[socket_fd]);
      output_queue = (*outputs)[socket_fd];
      while (!output_queue->empty()) {
        output = output_queue->front();
        output_queue->pop();
        delete output;
      }
      delete output_queue;
      telnet_ctxs->erase(socket_fd);
      outputs->erase(socket_fd);
  		break;
  	default:
  		/* ignore */
  		break;
	}
}

void telnet_event_handler_wrapper (telnet_t *telnet, telnet_event_t *event, void *user_data) {
  Telnet* server = ((user_data_t*)user_data)->server;
  server->telnet_event_handler(telnet, event, user_data);
}

telnet_t * Telnet::configure_telnet(int socket_fd) {
  using namespace std::placeholders;
  static const telnet_telopt_t telnet_options[] = {
//      { TELNET_TELOPT_ECHO,      TELNET_WILL, TELNET_DO },
//      { TELNET_TELOPT_TTYPE,     TELNET_WILL, TELNET_DO },
      { TELNET_TELOPT_COMPRESS2, TELNET_WILL, TELNET_DO   },
//      { TELNET_TELOPT_ZMP,       TELNET_WONT, TELNET_DO   },
//      { TELNET_TELOPT_MSSP,      TELNET_WONT, TELNET_DO   },
      { TELNET_TELOPT_BINARY,    TELNET_WILL, TELNET_DO   },
//      { TELNET_TELOPT_NAWS,      TELNET_WILL, TELNET_DONT },
      { -1, 0, 0 }
    };
    static user_data_t user_data;
    user_data.socket_fd = socket_fd;
    user_data.server = this;
    return telnet_init(telnet_options, telnet_event_handler_wrapper, 0, &user_data);
}

void Telnet::process_command(telnet_t *telnet, const char *buffer, size_t buffer_size) {
  std::string input = std::string(buffer, buffer_size);
  input.erase(std::remove(input.begin(), input.end(), '\r'), input.end()); //Cleaning input of CR & LF
  input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());
  if (input != "") {
    size_t pos = input.find_first_of(' ');
    std::string command;
    if (pos == std::string::npos)
      command = input;
    else
      command = input.substr(0, pos);
    if (telnet_resources->find(command) == telnet_resources->end())
      telnet_printf(telnet, "Unrecognized command.");
    else {
      std::string output = (*telnet_resources)[command]->process_command(input);
      telnet_send(telnet, output.c_str(), output.size());
    }
  }
  telnet_iac(telnet, '\xf9');
}
