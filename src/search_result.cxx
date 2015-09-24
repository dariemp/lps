#include "search_result.hxx"

using namespace search;

void SearchResult::insert(unsigned int rate_table_id, double rate) {
  tbb::mutex::scoped_lock lock(search_insertion_mutex);
  if (data.empty())
    data.emplace_back(new search_result_pair_t(rate_table_id, rate));
  size_t i = 0;
  size_t data_length = data.size();
  while (i <  data_length && rate < data[i]->second) i++;
  if (i < data_length) {
    if (rate_table_id != data[i]->first)
      data.emplace(data.begin() + i, new search_result_pair_t(rate_table_id, rate));
  }
  else
    data.emplace_back(new search_result_pair_t(rate_table_id, rate));
}

std::string SearchResult::to_json() {
  std::string json = "{[\n";
  for (auto it = data.begin(); it != data.end(); ++it)
    json += "\t[" + std::to_string((*it)->first) + ", " + std::to_string((*it)->second) + "],\n";
  json += "]}";
  return json;
}

std::string SearchResult::to_text_table() {
  return "";
}
