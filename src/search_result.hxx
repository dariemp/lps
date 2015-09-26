#ifndef SEARCH_RESULT_H
#define SEARCH_RESULT_H

#include <memory>
#include <vector>
#include <utility>
#include <tbb/tbb.h>

namespace search {

  class SearchResultElement;
  typedef std::vector<std::unique_ptr<SearchResultElement>> search_result_elements_t;
  static tbb::mutex search_insertion_mutex;

  class SearchResultElement {
    private:
      unsigned int rate_table_id;
      double rate;
      time_t effective_date;
      time_t end_date;
    public:
      SearchResultElement(unsigned int rate_table_id, double rate, time_t effective_date, time_t end_date);
      unsigned int get_rate_table_id();
      double get_rate();
      time_t get_effective_date();
      time_t get_end_date();
  };

  class SearchResult {
    private:
      search_result_elements_t data;
      void convert_dates(time_t data_effective_date, time_t data_end_date, std::string &effective_date, std::string &end_date);
    public:
      void insert(unsigned int rate_table_id, double rate, time_t effective_date, time_t end_date);
      std::string to_json();
      std::string to_text_table();
  };

}
#endif
