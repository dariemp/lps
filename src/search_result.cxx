#include "search_result.hxx"
#include <time.h>

using namespace search;

SearchResultElement::SearchResultElement(unsigned int rate_table_id, double rate, time_t effective_date, time_t end_date)
    : rate_table_id(rate_table_id), rate(rate), effective_date(effective_date), end_date(end_date) {};

unsigned int SearchResultElement::get_rate_table_id() {
  return rate_table_id;
}

double SearchResultElement::get_rate() {
  return rate;
}

time_t SearchResultElement::get_effective_date() {
  return effective_date;
}

time_t SearchResultElement::get_end_date() {
  return end_date;
}

void SearchResult::insert(unsigned int rate_table_id, double rate, time_t effective_date, time_t end_date) {
  tbb::mutex::scoped_lock lock(search_insertion_mutex);
  if (data.empty())
    data.emplace_back(new SearchResultElement(rate_table_id, rate, effective_date, end_date));
  size_t i = 0;
  size_t data_length = data.size();
  while (i <  data_length && rate < data[i]->get_rate()) i++;
  if (i < data_length) {
    if (rate_table_id != data[i]->get_rate_table_id())
      data.emplace(data.begin() + i, new SearchResultElement(rate_table_id, rate, effective_date, end_date));
  }
  else
    data.emplace_back(new SearchResultElement(rate_table_id, rate, effective_date, end_date));
}

void SearchResult::convert_dates(time_t data_effective_date, time_t data_end_date, std::string &effective_date, std::string &end_date){
  const char *date_format = "%a %e %b %Y %T %z";
  char cstr_effective_date[100];
  strftime(cstr_effective_date, 100, date_format, gmtime(&data_effective_date));
  effective_date = std::string(cstr_effective_date);
  end_date = "";
  if (data_end_date != -1) {
    char cstr_end_date[100];
    strftime(cstr_end_date, 100, date_format, gmtime(&data_end_date));
    end_date = std::string(cstr_end_date);
  }
}

std::string SearchResult::to_json() {
  std::string json = "{  \"rank\" : [\n";
  for (auto it = data.begin(); it != data.end(); ++it) {
    time_t data_effective_date = (*it)->get_effective_date();
    time_t data_end_date = (*it)->get_end_date();
    std::string effective_date;
    std::string end_date;
    convert_dates(data_effective_date, data_end_date, effective_date, end_date);
    if (it != data.begin() )
        json += ",\n";
    json += "\t\t{\n";
    json += "\t\t\t\"rate_table_id\" : " + std::to_string((*it)->get_rate_table_id()) + ",\n";
    json += "\t\t\t\"rate\" : " + std::to_string((*it)->get_rate()) + ",\n";
    json += "\t\t\t\"effective_date\" : \"" + effective_date + "\",\n";
    json += "\t\t\t\"end_date\" : \"" + end_date + "\"\n";
    json += "\t\t}";
  }
  json += "\n\t]\n}\n";
  return json;
}

std::string SearchResult::to_text_table() {
  std::string table = "";
  for (auto it = data.begin(); it != data.end(); ++it) {
    time_t data_effective_date = (*it)->get_effective_date();
    time_t data_end_date = (*it)->get_end_date();
    std::string effective_date;
    std::string end_date;
    convert_dates(data_effective_date, data_end_date, effective_date, end_date);
    table += std::to_string((*it)->get_rate_table_id()) + "  " + std::to_string((*it)->get_rate()) + "  " + effective_date + "  " + end_date + "\n";
  }
  return table;
}
