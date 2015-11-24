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

TelnetResource::~TelnetResource() {}

std::string TelnetSearchCode::process_command(std::string input) {
  try {
    args_t args = get_args(input);
    if (args.size() < 1)
      return "Invalid arguments.";
    std::string arg_code = args[0];
    trie::rate_type_t rate_type = trie::rate_type_t::RATE_TYPE_DEFAULT;
    if (args.size() > 1) {
      std::string arg_rate_type = args[1];
      rate_type = trie::to_rate_type_t(arg_rate_type);
    }
    unsigned long long code = std::stoull(arg_code);
    if (std::to_string(code) != arg_code)
      return "Invalid code.";
    search::SearchResult result;
    ctrl::p_controller_t p_controller = ctrl::Controller::get_controller();
    p_controller->search_code(code, rate_type, result);
    return result.to_text_table();
  } catch (std::exception &e) {
    return "Error";
  }
}

std::string TelnetSearchCodeName::process_command(std::string input) {
  try {
    args_t args = get_args(input);
    if (args.size() < 1)
      return "Invalid arguments.";
    std::string code_name = args[0];
    trie::rate_type_t rate_type = trie::rate_type_t::RATE_TYPE_DEFAULT;
    if (args.size() > 1) {
      std::string arg_rate_type = args[1];
      rate_type = trie::to_rate_type_t(arg_rate_type);
    }
    search::SearchResult result;
    ctrl::p_controller_t p_controller = ctrl::Controller::get_controller();
    p_controller->search_code_name(code_name, rate_type, result);
    return result.to_text_table();
  } catch (std::exception &e) {
    return "Error";
  }
}

std::string TelnetSearchCodeNameAndRateTable::process_command(std::string input) {
  try {
    args_t args = get_args(input);
    if (args.size() < 2)
      return "Invalid arguments.";
    std::string code_name = args[0];
    std::string arg_rate_table_id = args[1];
    trie::rate_type_t rate_type = trie::rate_type_t::RATE_TYPE_DEFAULT;
    if (args.size() > 2) {
      std::string arg_rate_type = args[2];
      rate_type = trie::to_rate_type_t(arg_rate_type);
    }
    unsigned int rate_table_id = stoul(arg_rate_table_id);
    if (std::to_string(rate_table_id) != arg_rate_table_id)
      return "Invalid rate table id.";
    search::SearchResult result;
    ctrl::p_controller_t p_controller = ctrl::Controller::get_controller();
    p_controller->search_code_name_rate_table(code_name, rate_table_id, rate_type, result);
    return result.to_text_table();
  } catch (std::exception &e) {
    return "Error";
  }
}

std::string TelnetSearchRateTable::process_command(std::string input) {
  try {
    args_t args = get_args(input);
    if (args.size() < 1)
      return "Invalid arguments.";
    std::string arg_rate_table_id = args[0];
    trie::rate_type_t rate_type = trie::rate_type_t::RATE_TYPE_DEFAULT;
    if (args.size() > 1) {
      std::string arg_rate_type = args[1];
      rate_type = trie::to_rate_type_t(arg_rate_type);
    }
    unsigned int rate_table_id = stoul(arg_rate_table_id);
    if (std::to_string(rate_table_id) != arg_rate_table_id)
      return "Invalid rate table id.";
    search::SearchResult result;
    ctrl::p_controller_t p_controller = ctrl::Controller::get_controller();
    p_controller->search_rate_table(rate_table_id, rate_type, result);
    return result.to_text_table();
  } catch (std::exception &e) {
    return "Error";
  }
}

std::string TelnetSearchAllCodes::process_command(std::string input) {
  try {
    trie::rate_type_t rate_type = trie::rate_type_t::RATE_TYPE_DEFAULT;
    args_t args = get_args(input);
    if (args.size() > 0) {
      std::string arg_rate_type = args[0];
      rate_type = trie::to_rate_type_t(arg_rate_type);
    }
    search::SearchResult result;
    ctrl::p_controller_t p_controller = ctrl::Controller::get_controller();
    p_controller->search_all_codes(rate_type, result);
    return result.to_text_table();
  } catch (std::exception &e) {
    return "Error";
  }
}
