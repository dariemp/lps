#ifndef SEARCH_RESULT_HXX
#define SEARCH_RESULT_HXX

#include "shared.hxx"
#include <tbb/tbb.h>
#include <set>

namespace search {

  class SearchResultElement {
    private:
      unsigned long long code;
      std::string code_name;
      unsigned long long rate_table_id;
      trie::rate_type_t rate_type;
      double current_min_rate;
      double current_max_rate;
      double future_min_rate;
      double future_max_rate;
      time_t effective_date;
      time_t end_date;
      time_t future_effective_date;
      time_t future_end_date;
      unsigned int egress_trunk_id;
    public:
      SearchResultElement(unsigned long long code,
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
                          unsigned int egress_trunk_id);
      bool operator >(SearchResultElement &other) const ;
      unsigned long long get_code();
      std::string get_code_name();
      unsigned long long get_rate_table_id();
      trie::rate_type_t get_rate_type();
      double get_current_min_rate();
      double get_current_max_rate();
      double get_future_min_rate();
      double get_future_max_rate();
      time_t get_effective_date();
      time_t get_end_date();
      time_t get_future_effective_date();
      time_t get_future_end_date();
      unsigned int get_egress_trunk_id();
  };

  typedef struct {
    bool operator ()(SearchResultElement* a, SearchResultElement* b) { return *a > *b; };
  } compare_elements_t;

  class SearchResult {
    private:
      typedef std::set<SearchResultElement*, compare_elements_t> search_result_elements_t;
      typedef search_result_elements_t* p_search_result_elements_t;
      unsigned int days_ahead;
      tbb::mutex search_insertion_mutex;
      p_search_result_elements_t data;
      void convert_date(time_t epoch_date, std::string &readable_date);
    public:
      SearchResult(unsigned int days_ahead = 7);
      ~SearchResult();
      void insert(unsigned long long code,
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
                  unsigned int egress_trunk_id);
      size_t size() const;
      std::string to_json(bool sumarize_rate_table = true);
      std::string to_text_table(bool sumarize_rate_table = true);
  };

}
#endif
