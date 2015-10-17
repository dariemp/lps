/**
      Prefix Tree (also known as Trie)
*/
#ifndef TRIE_H
#define TRIE_H

#include "search_result.hxx"
#include "shared.hxx"
#include <tbb/tbb.h>
#include <time.h>

namespace trie {

  class TrieData;
  class Trie;
  enum trie_data_field_t {
    CURRENT_DEFAULT_RATE             = 0x80000000,
    CURRENT_INTER_RATE               = 0x40000000,
    CURRENT_INTRA_RATE               = 0x20000000,
    CURRENT_LOCAL_RATE               = 0x10000000,
    CURRENT_DEFAULT_EFFECTIVE_DATE   = 0x08000000,
    CURRENT_DEFAULT_END_DATE         = 0x04000000,
    CURRENT_INTER_EFFECTIVE_DATE     = 0x02000000,
    CURRENT_INTER_END_DATE           = 0x01000000,
    CURRENT_INTRA_EFFECTIVE_DATE     = 0x00800000,
    CURRENT_INTRA_END_DATE           = 0x00400000,
    CURRENT_LOCAL_EFFECTIVE_DATE     = 0x00200000,
    CURRENT_LOCAL_END_DATE           = 0x00100000,
    FUTURE_DEFAULT_RATE              = 0x00080000,
    FUTURE_INTER_RATE                = 0x00040000,
    FUTURE_INTRA_RATE                = 0x00020000,
    FUTURE_LOCAL_RATE                = 0x00010000,
    FUTURE_DEFAULT_EFFECTIVE_DATE    = 0x00008000,
    FUTURE_DEFAULT_END_DATE          = 0x00004000,
    FUTURE_INTER_EFFECTIVE_DATE      = 0x00002000,
    FUTURE_INTER_END_DATE            = 0x00001000,
    FUTURE_INTRA_EFFECTIVE_DATE      = 0x00000800,
    FUTURE_INTRA_END_DATE            = 0x00000400,
    FUTURE_LOCAL_EFFECTIVE_DATE      = 0x00000200,
    FUTURE_LOCAL_END_DATE            = 0x00000100,
    TABLE_INDEX                      = 0x00000080,
    RATE_TABLE_ID                    = 0x00000040,
    CODE_NAME_ADDRESS                = 0x00000020,
    EMPTY_FLAG                       = 0x00000010,
  };

  //typedef std::unordered_map<unsigned char, void*> trie_data_fields_t;
  //typedef trie_data_fields_t* p_trie_data_fields_t;
  typedef TrieData* p_trie_data_t;
  typedef Trie* p_trie_t;
  typedef std::vector<trie::p_trie_t> tries_t;
  typedef std::unordered_map<unsigned char, p_trie_t> children_t;
  typedef tries_t* p_tries_t;
  typedef struct {
    unsigned char child_index;
    p_trie_t trie;
  } search_node_t;
  typedef search_node_t* p_search_node_t;
  typedef std::vector<p_search_node_t> search_nodes_t;



  class TrieData {
    private:
      //p_trie_data_fields_t fields;
      //trie_data_fields_t fields;
      uint32_t fields_bitmap;
      unsigned char fields_size;
      void** fields;
      unsigned char key_to_field_pos(trie_data_field_t key);
      template <typename T> T get_field(trie_data_field_t key);
      template <typename T> void set_field(trie_data_field_t key, T value);
      //void set_time_field(trie_data_field_t key, time_t value);
    public:
      TrieData();
      ~TrieData();
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
      size_t get_table_index();
      void set_table_index(size_t table_index);
      std::string get_code_name();
      void set_code_name_ptr(ctrl::p_code_pair_t code_name_ptr);
      unsigned int get_rate_table_id();
      void set_rate_table_id(unsigned int rate_table_id);
  };


static tbb::mutex trie_insertion_mutex;
  /**
      Prefix tree (trie) that stores rates and timestamps
  */
  class Trie {
    private:
      p_trie_data_t data;
      uint16_t children_bitmap;
      unsigned char children_size;
      p_trie_t* children;
      uint16_t index_to_mask(unsigned char index);
      unsigned char index_to_child_pos(unsigned char index);
      p_trie_data_t get_data();
      bool has_child(unsigned char index);
      p_trie_t get_child(unsigned char index);
      p_trie_t insert_child(unsigned char index);
      void set_current_data(rate_type_t rate_type, double rate, time_t effective_date, time_t end_date, ctrl::p_code_pair_t code_name_ptr);
      void set_future_data(rate_type_t rate_type, double rate, time_t effective_date, time_t end_date, ctrl::p_code_pair_t code_name_ptr);
      static void total_search_update_vars(const p_trie_t &current_trie,
                                           const search_nodes_t &nodes,
                                           unsigned long long &code,
                                           std::string &code_name,
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
      ~Trie();
      static void insert_code(const p_trie_t trie,
                              const char *code,
                              size_t code_length,
                              ctrl::p_code_pair_t code_name_ptr,
                              unsigned int rate_table_id,
                              double default_rate,
                              double inter_rate,
                              double intra_rate,
                              double local_rate,
                              time_t effective_date,
                              time_t end_date,
                              time_t reference_time);
      static void search_code(const p_trie_t trie, const char *code, size_t code_length, rate_type_t rate_type, search::SearchResult &search_result);
      //static void total_search_code(const p_trie_t trie, rate_type_t rate_type, search::SearchResult &search_result);
      static void insert_table_index(const p_trie_t trie, const char *prefix, size_t prefix_length, size_t index);
      static size_t search_table_index(const p_trie_t trie, const char *prefix, size_t prefix_length);
  };


}
#endif
