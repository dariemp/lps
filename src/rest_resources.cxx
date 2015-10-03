#include "rest_resources.hxx"
#include "controller.hxx"
#include <iostream>

using namespace rest;

void RestSearchCode::render(const http_request& request, http_response** response) {
  std::string code = request.get_arg("code");
  std::cout << "Procesing code: " << code << std::endl;
  search::SearchResult result;
  ctrl::p_controller_t p_controller = ctrl::Controller::get_controller();
  p_controller->search_code(code, result);
  *response = new http_response(http_response_builder(result.to_json(), 200, "application/json").string_response());
}

void RestSearchCodeName::render(const http_request& request, http_response** response) {

}

void RestSearchCodeNameAndRateTable::render(const http_request& request, http_response** response) {

}

void RestSearchRateTable::render(const http_request& request, http_response** response) {

}
