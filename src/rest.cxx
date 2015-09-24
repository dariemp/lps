#include "rest.hxx"
#include "controller.hxx"
#include <iostream>

using namespace rest;

void Rest::render(const http_request& request, http_response** response) {
  std::string prefix = request.get_arg("prefix");
  std::cout << "Procesing prefix: " << prefix << std::endl;
  search::SearchResult result;
  ctrl::p_controller_t p_controller = ctrl::Controller::get_controller();
  p_controller->search_prefix(prefix, result);
  *response = new http_response(http_response_builder(result.to_json(), 200, "application/json").string_response());
}

void Rest::run_server(unsigned int http_listen_port) {
  std::cout << "Listening HTTP server at port " << http_listen_port << "..." << std::endl;
  webserver server = create_webserver(http_listen_port).max_threads(10);
  server.register_resource("/search", this, true);
  server.start(true);
}
