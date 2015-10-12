#ifndef SEARCH_RESULT_H
#define SEARCH_RESULT_H

#include "shared.hxx"
#include <memory>
#include <vector>
#include <utility>
#include <tbb/tbb.h>

namespace search {

  class SearchResultElement;
  typedef std::vector<std::unique_ptr<SearchResultElement>> search_result_elements_t;

  class SearchResultElement {
    private:
      unsigned long long code;
      std::string code_name;
      unsigned int rate_table_id;
      trie::rate_type_t rate_type;
      double current_min_rate;
      double current_max_rate;
      double future_min_rate;
      double future_max_rate;
      time_t effective_date;
      time_t end_date;
      time_t future_effective_date;
      time_t future_end_date;
    public:
      SearchResultElement(unsigned long long code,
                          std::string code_name,
                          unsigned int rate_table_id,
                          trie::rate_type_t rate_type,
                          double current_min_rate,
                          double current_max_rate,
                          double future_min_rate,
                          double future_max_rate,
                          time_t effective_date,
                          time_t end_date,
                          time_t future_effective_date,
                          time_t future_end_date);
      unsigned long long get_code();
      std::string get_code_name();
      unsigned int get_rate_table_id();
      trie::rate_type_t get_rate_type();
      double get_current_min_rate();
      double get_current_max_rate();
      double get_future_min_rate();
      double get_future_max_rate();
      time_t get_effective_date();
      time_t get_end_date();
      time_t get_future_effective_date();
      time_t get_future_end_date();
  };

  class SearchResult {
    private:
      tbb::mutex search_insertion_mutex;
      search_result_elements_t data;
      void convert_date(time_t epoch_date, std::string &readable_date);
    public:
      SearchResultElement* operator [](size_t index) const;
      void insert(unsigned long long code,
                  std::string code_name,
                  unsigned int rate_table_id,
                  trie::rate_type_t rate_type,
                  double current_min_rate,
                  double current_max_rate,
                  double future_min_rate,
                  double future_max_rate,
                  time_t effective_date,
                  time_t end_date,
                  time_t future_effective_date,
                  time_t future_end_date);
      size_t size() const;
      std::string to_json();
      std::string to_text_table();
  };

}
#endif
