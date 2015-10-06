#include "rest.hxx"
#include "rest_resources.hxx"
#include <iostream>

using namespace rest;

void Rest::run_server(unsigned int http_listen_port) {
  std::cout << "Listening HTTP server at port " << http_listen_port << "..." << std::endl;
  webserver server = create_webserver(http_listen_port).max_threads(10);
  RestSearchCode search_code;
  RestSearchCodeName search_code_name;
  RestSearchCodeNameAndRateTable search_code_name_rate_table;
  RestSearchRateTable search_rate_table;
  RestSearchAllCodeNames search_all_codes;
  server.register_resource("/search_code", &search_code, true);
  server.register_resource("/search_code_name", &search_code_name, true);
  server.register_resource("/search_code_name_rate_table", &search_code_name_rate_table, true);
  server.register_resource("/search_rate_table", &search_rate_table, true);
  server.register_resource("/search_all_codes", &search_all_codes, true);
  server.start(true);
}
