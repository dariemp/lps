#ifndef SHARED_HXX
#define SHARED_HXX

#include <tbb/tbb.h>
#include <set>

namespace ctrl {

  typedef struct {
    bool operator ()(const unsigned long long &a, const unsigned long long &b) const { return a > b; };
  } compare_codes_t;

  typedef std::set<unsigned long long, compare_codes_t> code_set_t;
  typedef code_set_t* p_code_set_t;
  typedef std::pair<unsigned int, p_code_set_t> code_value_t;
  typedef code_value_t* p_code_value_t;
  typedef std::pair<const std::string, p_code_value_t> code_pair_t;
  typedef code_pair_t* p_code_pair_t;

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
