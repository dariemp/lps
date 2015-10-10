/**
      Prefix Tree (also known as Trie)
*/
#ifndef TRIE_H
#define TRIE_H

#include "search_result.hxx"
#include "shared.hxx"
#include <tbb/tbb.h>
#include <memory>
#include <time.h>

namespace trie {

  class TrieData;
  class Trie;
  typedef TrieData* p_trie_data_t;
  typedef std::unique_ptr<Trie> uptr_trie_t;
  typedef Trie* p_trie_t;
  typedef std::vector<uptr_trie_t> children_tries_t;
  typedef struct {
    unsigned char child_index;
    p_trie_t trie;
  } search_node_t;
  typedef search_node_t* p_search_node_t;
  typedef std::vector<std::unique_ptr<search_node_t>> search_nodes_t;

  class TrieData {
    private:
      double default_rate;
      double inter_rate;
      double intra_rate;
      double local_rate;
      time_t default_effective_date;
      time_t default_end_date;
      time_t inter_effective_date;
      time_t inter_end_date;
      time_t intra_effective_date;
      time_t intra_end_date;
      time_t local_effective_date;
      time_t local_end_date;
    public:
      TrieData();
      double get_rate(rate_type_t rate_type);
      time_t get_effective_date(rate_type_t rate_type);
      time_t get_end_date(rate_type_t rate_type);
      void set_rate(rate_type_t rate_type, double rate);
      void set_effective_date(rate_type_t rate_type, time_t effective_date);
      void set_end_date(rate_type_t rate_type, time_t end_date);
  };


static tbb::mutex trie_insertion_mutex;
  /**
      Prefix tree (trie) that stores rates and timestamps
  */
  class Trie {
    private:
      TrieData data;
      uptr_trie_t children[10];
      static void total_search_update_vars(const p_trie_t &current_trie,
                                           const search_nodes_t &nodes,
                                           unsigned long long &code,
                                           const rate_type_t rate_type,
                                           double &current_min_rate,
                                           double &current_max_rate,
                                           double &future_min_rate,
                                           double &future_max_rate,
                                           time_t &effective_date,
                                           time_t &end_date,
                                           time_t &future_effective_date,
                                           time_t &future_end_date);
    public:
      Trie();
      p_trie_data_t get_data();
      void set_data(rate_type_t rate_type, double rate, time_t effective_date, time_t end_date);
      p_trie_t get_child(unsigned char index);
      p_trie_t insert_child(unsigned char index);
      bool has_child(unsigned char index);
      static void insert(const p_trie_t trie, const char *prefix, size_t prefix_length, double default_rate, double inter_rate, double intra_rate, double local_rate, time_t effective_date, time_t end_date);
      static void search(const p_trie_t trie, const char *prefix, size_t prefix_length, rate_type_t rate_type, unsigned int rate_table_id, ctrl::p_code_names_t code_names, search::SearchResult &result);
      static void total_search(const p_trie_t trie, rate_type_t rate_type, unsigned int rate_table_id, ctrl::p_code_names_t code_names, search::SearchResult &search_result);
  };


}
#endif
