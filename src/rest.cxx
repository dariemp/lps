#include "rest.hxx"
#include "rest_resources.hxx"
#include <iostream>

using namespace rest;

void Rest::run_server(unsigned int http_listen_port) {
  std::cout << "Listening HTTP server at port " << http_listen_port << "..." << std::endl;
  webserver server = create_webserver(http_listen_port).max_threads(10);
  RestSearchCode search_code;
  server.register_resource("/search_code", &search_code, true);
  server.start(true);
}
