#include "rest_resources.hxx"
#include "controller.hxx"
#include <iostream>

using namespace rest;

void RestSearchCode::render(const http_request& request, http_response** response) {
  try {
    std::string arg_code = request.get_arg("code");
    std::string arg_rate_type = request.get_arg("rate_type");
    std::string arg_days_ahead = request.get_arg("days_ahead");
    trie::rate_type_t rate_type = trie::to_rate_type_t(arg_rate_type);
    unsigned int days_ahead = arg_days_ahead == "" ? 7 : std::stoul(arg_days_ahead);
    search::SearchResult result(days_ahead);
    ctrl::p_controller_t p_controller = ctrl::Controller::get_controller();
    unsigned long long code = stoull(arg_code);
    p_controller->search_code(code, rate_type, result);
    *response = new http_response(http_response_builder(result.to_json(), 200, "application/json").string_response());
  } catch (std::exception &e) {
    *response = new http_response(http_response_builder("Error", 400, "text/plain").string_response());
  }
}

void RestSearchCodeName::render(const http_request& request, http_response** response) {
  try {
    std::string code_name = request.get_arg("code_name");
    std::string arg_rate_type = request.get_arg("rate_type");
    std::string arg_days_ahead = request.get_arg("days_ahead");
    trie::rate_type_t rate_type = trie::to_rate_type_t(arg_rate_type);
    unsigned int days_ahead = arg_days_ahead == "" ? 7 : std::stoul(arg_days_ahead);
    search::SearchResult result(days_ahead);
    ctrl::p_controller_t p_controller = ctrl::Controller::get_controller();
    p_controller->search_code_name(code_name, rate_type, result);
    *response = new http_response(http_response_builder(result.to_json(), 200, "application/json").string_response());
  } catch (std::exception &e) {
    *response = new http_response(http_response_builder("Error", 400, "text/plain").string_response());
  }
}

void RestSearchCodeNameAndRateTable::render(const http_request& request, http_response** response) {
  try {
    std::string code_name = request.get_arg("code_name");
    std::string arg_rate_table_id = request.get_arg("rate_table_id");
    std::string arg_rate_type = request.get_arg("rate_type");
    std::string arg_days_ahead = request.get_arg("days_ahead");
    trie::rate_type_t rate_type = trie::to_rate_type_t(arg_rate_type);
    unsigned int days_ahead = arg_days_ahead == "" ? 7 : std::stoul(arg_days_ahead);
    search::SearchResult result(days_ahead);
    ctrl::p_controller_t p_controller = ctrl::Controller::get_controller();
    unsigned int rate_table_id = stoul(arg_rate_table_id);
    p_controller->search_code_name_rate_table(code_name, rate_table_id, rate_type, result, !sumarize_rate_table);
    *response = new http_response(http_response_builder(result.to_json(sumarize_rate_table), 200, "application/json").string_response());
  } catch (std::exception &e) {
    *response = new http_response(http_response_builder("Error", 400, "text/plain").string_response());
  }

}

void RestSearchRateTable::render(const http_request& request, http_response** response) {
  try {
    std::string arg_rate_table_id = request.get_arg("rate_table_id");
    std::string arg_rate_type = request.get_arg("rate_type");
    std::string arg_days_ahead = request.get_arg("days_ahead");
    trie::rate_type_t rate_type = trie::to_rate_type_t(arg_rate_type);
    unsigned int days_ahead = arg_days_ahead == "" ? 7 : std::stoul(arg_days_ahead);
    search::SearchResult result(days_ahead);
    ctrl::p_controller_t p_controller = ctrl::Controller::get_controller();
    unsigned int rate_table_id = stoul(arg_rate_table_id);
    p_controller->search_rate_table(rate_table_id, rate_type, result);
    *response = new http_response(http_response_builder(result.to_json(), 200, "application/json").string_response());
  } catch (std::exception &e) {
    *response = new http_response(http_response_builder("Error", 400, "text/plain").string_response());
  }
}

void RestSearchAllCodeNames::render(const http_request& request, http_response** response) {
  try {
    std::string arg_rate_type = request.get_arg("rate_type");
    trie::rate_type_t rate_type = trie::to_rate_type_t(arg_rate_type);
    std::string arg_days_ahead = request.get_arg("days_ahead");
    unsigned int days_ahead = arg_days_ahead == "" ? 7 : std::stoul(arg_days_ahead);
    search::SearchResult result(days_ahead);
    ctrl::p_controller_t p_controller = ctrl::Controller::get_controller();
    p_controller->search_all_codes(rate_type, result);
    *response = new http_response(http_response_builder(result.to_json(), 200, "application/json").string_response());
  } catch (std::exception &e) {
    *response = new http_response(http_response_builder("Error", 400, "text/plain").string_response());
  }
}
