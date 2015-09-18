/**
      Prefix Tree (also known as Trie)
*/
#ifndef TRIE_H
#define TRIE_H

#include <memory>
#include <time.h>

namespace trie {
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
  class Trie {
    private:
      std::shared_ptr<TrieData> data;
      std::shared_ptr<Trie> children[10];
    public:
      Trie();
      std::shared_ptr<TrieData> get_data();
      void set_data(std::shared_ptr<TrieData> new_data);
      std::shared_ptr<Trie> get_child(unsigned char index);
      bool has_child_data(unsigned char index);
      static void insert(std::shared_ptr<Trie> trie, const char *prefix, size_t prefix_length, std::shared_ptr<TrieData> new_data);
      static std::shared_ptr<TrieData> search(std::shared_ptr<Trie> trie, const char *prefix, size_t prefix_length);
  };
}
#endif
