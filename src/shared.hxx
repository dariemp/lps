#ifndef SHARED_H
#define SHARED_H
#include <unordered_map>
#include <memory>

namespace ctrl {
  typedef std::unordered_map<unsigned long long, std::string> code_names_t;
  typedef code_names_t* p_code_names_t;
  typedef std::unique_ptr<code_names_t> uptr_code_names_t;

  typedef std::unordered_map<std::string, unsigned long long> codes_t;
  typedef codes_t* p_codes_t;
  typedef std::unique_ptr<codes_t> uptr_codes_t;
}

namespace trie {
  enum rate_type_t {
    RATE_TYPE_DEFAULT,
    RATE_TYPE_INTER,
    RATE_TYPE_INTRA,
    RATE_TYPE_LOCAL
  };

  rate_type_t to_rate_type_t(std::string rate_type);
}

#endif
