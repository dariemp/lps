#ifndef SEARCH_RESULT_H
#define SEARCH_RESULT_H

#include <memory>
#include <vector>
#include <utility>
#include <tbb/tbb.h>

namespace search {

  typedef std::pair<unsigned int, double> search_result_pair_t;
  typedef std::vector<std::unique_ptr<search_result_pair_t>> search_result_pairs_t;
  static tbb::mutex search_insertion_mutex;

  class SearchResult {
    private:
      search_result_pairs_t data;
    public:
      void insert(unsigned int rate_table_id, double rate);
      std::string to_json();
      std::string to_text_table();
  };

}
#endif
