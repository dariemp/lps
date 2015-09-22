/**
      Prefix Tree (also known as Trie)
*/
#ifndef TRIE_H
#define TRIE_H

#include <tbb/tbb.h>
#include <memory>
#include <time.h>

namespace trie {

  class TrieData;
  class Trie;
  //typedef std::unique_ptr<TrieData> uptr_trie_data_t;
  typedef TrieData* p_trie_data_t;
  typedef std::unique_ptr<Trie> uptr_trie_t;
  typedef Trie* p_trie_t;
  typedef std::vector<uptr_trie_t> children_tries_t;

  class TrieData {
    private:
      double rate;
      time_t effective_date;
      time_t end_date;
    public:
      TrieData();
      double get_rate();
      time_t get_effective_date();
      time_t get_end_date();
      void set_rate(double rate);
      void set_effective_date(time_t effective_date);
      void set_end_date(time_t end_date);
  };

  /**
      Prefix tree (trie) that stores rates and timestamps
  */
  static tbb::mutex trie_insertion_mutex;

  class Trie {
    private:
      TrieData data;
      children_tries_t children;
    public:
      //Trie();
      p_trie_data_t get_data();
      void set_data(double rate, time_t effective_date, time_t end_date);
      p_trie_t get_child(unsigned char index);
      bool has_child_data(unsigned char index);
      static void insert(p_trie_t trie, const char *prefix, size_t prefix_length, double rate, time_t effective_date, time_t end_date);
      static p_trie_data_t search(p_trie_t trie, const char *prefix, size_t prefix_length);
  };


}
#endif
