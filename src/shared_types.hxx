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
