#include "search_result.hxx"
#include "code.hxx"
#include <time.h>
#include <iostream>
#include <chrono>

using namespace search;

SearchResultElement::SearchResultElement(unsigned long long code, std::string code_name, unsigned long long rate_table_id, trie::rate_type_t rate_type,
      double current_min_rate, double current_max_rate, double future_min_rate, double future_max_rate,
      time_t effective_date, time_t end_date, time_t future_effective_date, time_t future_end_date, unsigned int egress_trunk_id)
        : code(code),
          code_name(code_name),
          rate_table_id(rate_table_id),
          rate_type(rate_type),
          current_min_rate(current_min_rate),
          current_max_rate(current_max_rate),
          future_min_rate(future_min_rate),
          future_max_rate(future_max_rate),
          effective_date(effective_date),
          end_date(end_date),
          future_effective_date(future_effective_date),
          future_end_date(future_end_date),
          egress_trunk_id(egress_trunk_id) {}

bool SearchResultElement::operator >(SearchResultElement &other) const {
  if (this->current_max_rate > other.current_max_rate)
    return true;
  else if (this->current_max_rate == other.current_max_rate) {
    if (this->code > other.code)
      return true;
    else if (this->code == other.code && this->rate_table_id > other.rate_table_id)
        return true;
  }
  return false;
}

unsigned long long SearchResultElement::get_code() {
    return code;
}

std::string SearchResultElement::get_code_name() {
  return code_name;
}

unsigned long long SearchResultElement::get_rate_table_id() {
  return rate_table_id;
}

trie::rate_type_t SearchResultElement::get_rate_type() {
  return rate_type;
}

double SearchResultElement::get_current_min_rate() {
  return current_min_rate;
}

double SearchResultElement::get_current_max_rate() {
  return current_max_rate;
}

double SearchResultElement::get_future_min_rate() {
  return future_min_rate;
}

double SearchResultElement::get_future_max_rate() {
  return future_max_rate;
}

time_t SearchResultElement::get_effective_date() {
  return effective_date;
}

time_t SearchResultElement::get_end_date() {
  return end_date;
}

time_t SearchResultElement::get_future_effective_date() {
  return future_effective_date;
}

time_t SearchResultElement::get_future_end_date() {
  return future_end_date;
}

unsigned int SearchResultElement::get_egress_trunk_id() {
  return egress_trunk_id;
}

SearchResult::SearchResult(unsigned int days_ahead) : days_ahead(days_ahead) {
  data = new search_result_elements_t();
}

SearchResult::~SearchResult() {
  for (auto it = data->begin(); it != data->end(); ++it)
    delete *it;
  data->clear();
  delete data;
}

void SearchResult::insert(unsigned long long code,
                          std::string code_name,
                          unsigned long long rate_table_id,
                          trie::rate_type_t rate_type,
                          double current_min_rate,
                          double current_max_rate,
                          double future_min_rate,
                          double future_max_rate,
                          time_t effective_date,
                          time_t end_date,
                          time_t future_effective_date,
                          time_t future_end_date,
                          time_t reference_time,
                          unsigned int egress_trunk_id) {
  tbb::mutex::scoped_lock lock(search_insertion_mutex);
  if (future_effective_date != -1 && end_date == -1)
    end_date = future_effective_date - 1;
  time_t future_date = reference_time + 3600 * 24 * days_ahead;
  if (end_date <= 0) {
    future_max_rate = current_max_rate;
    future_min_rate = current_min_rate;
    future_effective_date = future_date;
    future_end_date =  -1;
  }
  else if (future_effective_date <= 0) {
    if (future_date > end_date)
      future_effective_date = future_date;
    else
      future_effective_date = end_date + 1;
    future_end_date = -1;
    future_max_rate = -1;
    future_min_rate = -1;
  }
  else {
    if (future_date > future_effective_date)
      future_effective_date = future_date;
    if (future_end_date > 0 && future_date > future_end_date) {
      future_end_date = -1;
      future_max_rate = -1;
      future_min_rate = -1;
    }
  }

  data->insert(new SearchResultElement(code, code_name, rate_table_id, rate_type, current_min_rate, current_max_rate,
                          future_min_rate, future_max_rate, effective_date, end_date,
                          future_effective_date, future_end_date, egress_trunk_id));
}

void SearchResult::convert_date(time_t epoch_date, std::string &readable_date) {
  readable_date = "";
  if (epoch_date > 0) {
    char buffer_date[100];
    strftime(buffer_date, 100, "%Y-%m-%d %T%z", gmtime(&epoch_date));
    readable_date = std::string(buffer_date);
  }
}

std::string SearchResult::to_json(bool sumarize_rate_table) {
  std::set<unsigned int> rate_table_ids;
  std::string json = "{  \"rank\" : [\n";
  for (auto it = data->begin(); it != data->end(); ++it) {
    unsigned int rate_table_id = (*it)->get_rate_table_id();
    if (sumarize_rate_table) {
      if (rate_table_ids.find(rate_table_id) !=  rate_table_ids.end())
        continue;
      rate_table_ids.insert(rate_table_id);
    }
    time_t epoch_effective_date = (*it)->get_effective_date();
    time_t epoch_end_date = (*it)->get_end_date();
    std::string effective_date;
    std::string end_date;
    convert_date(epoch_effective_date, effective_date);
    convert_date(epoch_end_date, end_date);
    double future_max_rate = (*it)->get_future_max_rate();
    if (it != data->begin() )
        json += ",\n";
    json += "\t\t{\n";
    unsigned long long code = (*it)->get_code();
    if (code != 0)
      json += "\t\t\t\"code\" : " + std::to_string(code) + ",\n";
    json += "\t\t\t\"code_name\" : \"" + (*it)->get_code_name() + "\",\n";
    json += "\t\t\t\"rate_type\" : \"" + trie::rate_type_to_string((*it)->get_rate_type()) + "\",\n";
    json += "\t\t\t\"rate_table_id\" : " + std::to_string(rate_table_id) + ",\n";
    json += "\t\t\t\"egress_trunk_id\" : " + std::to_string((*it)->get_egress_trunk_id()) + ",\n";
    json += "\t\t\t\"current_max_rate\" : " + std::to_string((*it)->get_current_max_rate()) + ",\n";
    json += "\t\t\t\"current_min_rate\" : " + std::to_string((*it)->get_current_min_rate()) + ",\n";
    json += "\t\t\t\"effective_date\" : \"" + effective_date + "\",\n";
    if (epoch_end_date <= 0)
      json += "\t\t\t\"end_date\" : null\n";
    else {
      convert_date(epoch_end_date, end_date);
      json += "\t\t\t\"end_date\" : \"" + end_date + "\"\n";
    }
    if (future_max_rate <= 0) {
      json += "\t\t\t\"future_max_rate\" : null,\n";
      json += "\t\t\t\"future_min_rate\" : null,\n";
      json += "\t\t\t\"future_effective_date\" : null,\n";
      json += "\t\t\t\"future_end_date\" : null\n";
    }
    else {
      double future_min_rate = (*it)->get_future_min_rate();
      time_t epoch_future_effective_date = (*it)->get_future_effective_date();
      time_t epoch_future_end_date = (*it)->get_future_end_date();
      std::string future_effective_date;
      std::string future_end_date;
      convert_date(epoch_future_effective_date, future_effective_date);
      json += "\t\t\t\"future_max_rate\" : " + std::to_string(future_max_rate) + ",\n";
      json += "\t\t\t\"future_min_rate\" : " + std::to_string(future_min_rate)  + ",\n";
      json += "\t\t\t\"future_effective_date\" : \"" + future_effective_date + "\",\n";
      if (epoch_future_end_date <= 0)
        json += "\t\t\t\"future_end_date\" : null\n";
      else {
        convert_date(epoch_future_end_date, future_end_date);
        json += "\t\t\t\"future_end_date\" : \"" + future_end_date + "\"\n";
      }
    }
    json += "\t\t}";
  }
  json += "\n\t]\n}\n";
  return json;
}

std::string SearchResult::to_text_table(bool sumarize_rate_table) {
  std::set<unsigned int> rate_table_ids;
  std::string table = "";
  for (auto it = data->begin(); it != data->end(); ++it) {
    unsigned int rate_table_id = (*it)->get_rate_table_id();
    if (sumarize_rate_table) {
      if (rate_table_ids.find(rate_table_id) !=  rate_table_ids.end())
        continue;
      rate_table_ids.insert(rate_table_id);
    }
    time_t epoch_effective_date = (*it)->get_effective_date();
    time_t epoch_end_date = (*it)->get_end_date();
    std::string effective_date;
    std::string end_date;
    convert_date(epoch_effective_date, effective_date);
    double future_max_rate = (*it)->get_future_max_rate();
    unsigned long long code = (*it)->get_code();
    if (code != 0)
      table += "code                  : " + std::to_string((*it)->get_code()) + "\n";
    table += "code_name             : " + (*it)->get_code_name() + "\n";
    table += "rate_type             : " + trie::rate_type_to_string((*it)->get_rate_type()) + "\n";
    table += "rate_table_id         : " + std::to_string((*it)->get_rate_table_id()) + "\n";
    table += "egress_trunk_id       : " + std::to_string((*it)->get_egress_trunk_id()) + "\n";
    table += "current_max_rate      : " + std::to_string((*it)->get_current_max_rate()) + "\n";
    table += "current_min_rate      : " + std::to_string((*it)->get_current_min_rate()) + "\n";
    table += "effective_date        : " + effective_date + "\n";
    if (epoch_end_date <= 0)
      table += "end_date              : -\n";
    else {
      convert_date(epoch_end_date, end_date);
      table += "end_date              : " + end_date + "\n";
    }
    if (future_max_rate <= 0) {
      table += "future_max_rate       : -\n";
      table += "future_min_rate       : -\n";
      table += "future_effective_date : -\n";
      table += "future_end_date       : -\n";
    }
    else {
      double future_min_rate = (*it)->get_future_min_rate();
      time_t epoch_future_effective_date = (*it)->get_future_effective_date();
      time_t epoch_future_end_date = (*it)->get_future_end_date();
      std::string future_effective_date;
      std::string future_end_date;
      convert_date(epoch_future_effective_date, future_effective_date);
      table += "future_max_rate       : " + std::to_string(future_max_rate) + "\n";
      table += "future_min_rate       : " + std::to_string(future_min_rate)  + "\n";
      table += "future_effective_date : " + future_effective_date + "\n";
      if (epoch_future_end_date <= 0)
        table += "future_end_date       : -\n";
      else {
        convert_date(epoch_future_end_date, future_end_date);
        table += "future_end_date       : " + future_end_date + "\n";
      }
    }
    table += "\n";
  }
  return table;
}

size_t SearchResult::size() const {
  return data->size();
}
