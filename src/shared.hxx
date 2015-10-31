#ifndef SHARED_HXX
#define SHARED_HXX

#include <tbb/tbb.h>
#include <thread>

namespace ctrl {
  typedef tbb::concurrent_vector<unsigned long long> code_list_t;
  typedef code_list_t* p_code_list_t;
  typedef tbb::concurrent_unordered_map<std::string, p_code_list_t> codes_t;
  typedef std::pair<const std::string, p_code_list_t> code_pair_t;
  typedef code_pair_t* p_code_pair_t;
  typedef codes_t* p_codes_t;

  void str_to_upper(std::string &str);
}

namespace trie {
  enum rate_type_t {
    RATE_TYPE_DEFAULT,
    RATE_TYPE_INTER,
    RATE_TYPE_INTRA,
    RATE_TYPE_LOCAL
  };

  rate_type_t to_rate_type_t(std::string rate_type);
  std::string rate_type_to_string(rate_type_t rate_type);
}

#endif
