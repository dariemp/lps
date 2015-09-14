/**
      Prefix Tree (also known as Trie)
*/
#ifndef TRIE_H
#define TRIE_H
#include <memory>
#include <algorithm>
#include "exceptions.hxx"

namespace trie {
/**
    Nodes of a linked list that stores ordered rate data in the nodes of the prefix tree
*/
  class RateTableData {
    private:
      unsigned int rate_table_id;
      double rate;
      std::shared_ptr<RateTableData> next;
    public:
      RateTableData(unsigned int rate_table_id, double rate);
      std::shared_ptr<RateTableData> get_next();
      void set_next(std::shared_ptr<RateTableData> next);
      unsigned int get_rate_table_id();
      double get_rate();
  };

/**
    Prefix tree (trie) that stores ordered rate data in its nodes as linked lists
*/
  class Trie {
    private:
      std::shared_ptr<RateTableData> data;
      std::shared_ptr<Trie> children[10];
      void insert_data(unsigned int rate_table_id, double rate);
    public:
      Trie();
      void insert(unsigned char *prefix, size_t prefix_length, unsigned int rate_table_id, double rate);
  };
}
#endif
