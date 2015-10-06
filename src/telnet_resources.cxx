#include "telnet_resources.hxx"
#include "controller.hxx"
#include <iostream>


using namespace telnet;

args_t TelnetResource::get_args(std::string input) {
  args_t args;
  args_t empty_args;
  size_t next_pos, first_pos = input.find_first_of(' ');
  if (first_pos == std::string::npos)
    return empty_args;
  ++first_pos;
  if (first_pos < input.size() && input[first_pos] == '(')
    next_pos = input.find_first_of(')', ++first_pos);
  while (next_pos != std::string::npos) {
    size_t len = next_pos - first_pos;
    args.push_back(input.substr(first_pos, len));
    first_pos = next_pos + 1;
    if (first_pos >= input.size())
      return args;
    else if (input[first_pos] == '(')
      next_pos = input.find_first_of(')', ++first_pos);
    else
      return empty_args;
  }
  return empty_args;
}

std::string TelnetSearchCode::process_command(std::string input) {
  args_t args = get_args(input);
  if (args.size() < 1)
    return "Invalid arguments.\n";
  std::string code = args[0];
  unsigned long long numeric_code = std::stoull(code);
  if (std::to_string(numeric_code) != code)
    return "Invaid code.\n";
  std::cout << "Procesing code: " << code << std::endl;
  search::SearchResult result;
  ctrl::p_controller_t p_controller = ctrl::Controller::get_controller();
  p_controller->search_code(code, result);
  return result.to_text_table();
}

std::string TelnetSearchCodeName::process_command(std::string input) {
  args_t args = get_args(input);
  if (args.size() < 1)
    return "Invalid arguments.\n";
  std::string code_name = args[0];
  std::cout << "Procesing code name: " << code_name << std::endl;
  search::SearchResult result;
  ctrl::p_controller_t p_controller = ctrl::Controller::get_controller();
  p_controller->search_code_name(code_name, result);
  return result.to_text_table();
}

std::string TelnetSearchCodeNameAndRateTable::process_command(std::string input) {
  args_t args = get_args(input);
  if (args.size() < 2)
    return "Invalid arguments.\n";
  std::string code_name = args[0];
  std::string rate_table_id = args[1];
  unsigned int numeric_rate_table_id = std::stoull(rate_table_id);
  if (std::to_string(numeric_rate_table_id) != rate_table_id)
    return "Invaid rate table ID.\n";
  std::cout << "Procesing code name: " << code_name << " and rate table ID: " << rate_table_id << std::endl;
  search::SearchResult result;
  ctrl::p_controller_t p_controller = ctrl::Controller::get_controller();
  p_controller->search_code_name_rate_table(code_name, numeric_rate_table_id, result);
  return result.to_text_table();
}

std::string TelnetSearchRateTable::process_command(std::string input) {
  args_t args = get_args(input);
  if (args.size() < 1)
    return "Invalid arguments.\n";
  std::string rate_table_id = args[0];
  unsigned int numeric_rate_table_id = std::stoull(rate_table_id);
  if (std::to_string(numeric_rate_table_id) != rate_table_id)
    return "Invaid rate table ID.\n";
  std::cout << "Procesing rate table ID: " << rate_table_id << std::endl;
  search::SearchResult result;
  ctrl::p_controller_t p_controller = ctrl::Controller::get_controller();
  p_controller->search_rate_table(numeric_rate_table_id, result);
  return result.to_text_table();
}

std::string TelnetSearchAllCodes::process_command(std::string input) {
  search::SearchResult result;
  ctrl::p_controller_t p_controller = ctrl::Controller::get_controller();
  p_controller->search_all_codes(result);
  return result.to_text_table();
}
