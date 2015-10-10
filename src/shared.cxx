#include "shared.hxx"
#include "exceptions.hxx"

using namespace trie;

rate_type_t trie::to_rate_type_t(std::string rate_type){
  if (rate_type == "inter")
    return rate_type_t::RATE_TYPE_INTER;
  else if (rate_type == "intra")
    return rate_type_t::RATE_TYPE_INTRA;
  else if (rate_type == "local")
    return rate_type_t::RATE_TYPE_LOCAL;
  else if (rate_type == "" || rate_type == "default")
    return rate_type_t::RATE_TYPE_DEFAULT;
  else
    throw RestRequestArgException();
}