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
      double current_default_rate;
      double current_inter_rate;
      double current_intra_rate;
      double current_local_rate;
      time_t current_default_effective_date;
      time_t current_default_end_date;
      time_t current_inter_effective_date;
      time_t current_inter_end_date;
      time_t current_intra_effective_date;
      time_t current_intra_end_date;
      time_t current_local_effective_date;
      time_t current_local_end_date;
      double future_default_rate;
      double future_inter_rate;
      double future_intra_rate;
      double future_local_rate;
      time_t future_default_effective_date;
      time_t future_default_end_date;
      time_t future_inter_effective_date;
      time_t future_inter_end_date;
      time_t future_intra_effective_date;
      time_t future_intra_end_date;
      time_t future_local_effective_date;
      time_t future_local_end_date;
    public:
      TrieData();
      double get_current_rate(rate_type_t rate_type);
      time_t get_current_effective_date(rate_type_t rate_type);
      time_t get_current_end_date(rate_type_t rate_type);
      void set_current_rate(rate_type_t rate_type, double rate);
      void set_current_effective_date(rate_type_t rate_type, time_t effective_date);
      void set_current_end_date(rate_type_t rate_type, time_t end_date);
      double get_future_rate(rate_type_t rate_type);
      time_t get_future_effective_date(rate_type_t rate_type);
      time_t get_future_end_date(rate_type_t rate_type);
      void set_future_rate(rate_type_t rate_type, double rate);
      void set_future_effective_date(rate_type_t rate_type, time_t effective_date);
      void set_future_end_date(rate_type_t rate_type, time_t end_date);
  };


static tbb::mutex trie_insertion_mutex;
  /**
      Prefix tree (trie) that stores rates and timestamps
  */
  class Trie {
    private:
      TrieData data;
      uptr_trie_t children[10];
      void set_current_data(rate_type_t rate_type, double rate, time_t effective_date, time_t end_date);
      void set_future_data(rate_type_t rate_type, double rate, time_t effective_date, time_t end_date);
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
      p_trie_t get_child(unsigned char index);
      p_trie_t insert_child(unsigned char index);
      bool has_child(unsigned char index);
      static void insert(const p_trie_t trie, const char *prefix, size_t prefix_length, double default_rate, double inter_rate, double intra_rate, double local_rate, time_t effective_date, time_t end_date, time_t reference_time);
      static void search(const p_trie_t trie, const char *prefix, size_t prefix_length, rate_type_t rate_type, unsigned int rate_table_id, ctrl::p_code_names_t code_names, search::SearchResult &result);
      static void total_search(const p_trie_t trie, rate_type_t rate_type, unsigned int rate_table_id, ctrl::p_code_names_t code_names, search::SearchResult &search_result);
  };


}
#endif
