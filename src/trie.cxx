#include "trie.hxx"
#include "code.hxx"
#include "exceptions.hxx"
#include "logger.hxx"
#include <algorithm>
#include <iostream>

using namespace trie;

TrieData::TrieData() {
  fields = nullptr;
  fields_bitmap = 0;
  fields_size = 0;
}

TrieData::~TrieData() {
  for (uint32_t key = CURRENT_DEFAULT_RATE; key != EMPTY_FLAG; key >>= 1)
    if ((fields_bitmap & key) && key != CODE_NAME_ADDRESS)
        free(fields[key_to_field_pos((trie_data_field_t)key)]);
  free(fields);
}

unsigned char TrieData::key_to_field_pos(trie_data_field_t key) {
  unsigned char pos = 0;
  uint32_t mask = key;
  uint32_t bitmap = fields_bitmap;
  while (mask != 0x80000000) {
    if (bitmap & 0x80000000) pos++;
    bitmap <<= 1;
    mask <<= 1;
  }
  return pos;
}

template <typename T> T TrieData::get_field(trie_data_field_t key) {
  if (fields_bitmap & key)
    return *(T*)fields[key_to_field_pos(key)];
  else
    return -1;
}

namespace trie {
  template <> unsigned int TrieData::get_field<unsigned int>(trie_data_field_t key) {
    if (fields_bitmap & key)
      return *(int*)fields[key_to_field_pos(key)];
    else
      return 0;
  }
};


template <typename T> void TrieData::set_field(trie_data_field_t key, T value) {
  if (value <= 0)
    return;
  unsigned char field_pos = key_to_field_pos(key);
  if ((fields_bitmap & key) == 0) {
    fields_size += sizeof(void*);
    fields = (void**)realloc(fields, fields_size);
    unsigned char count = fields_size / sizeof(void*);
    for (size_t i = count - 1; i > field_pos; --i)
      fields[i] = fields[i-1];
    fields[field_pos] = malloc(sizeof(T));
    fields_bitmap |= key;
  }
  *(T*)fields[field_pos] = value;
}

double TrieData::get_current_rate(rate_type_t rate_type) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      return get_field<double>(CURRENT_INTER_RATE);
      break;
    case RATE_TYPE_INTRA:
      return get_field<double>(CURRENT_INTRA_RATE);
      break;
    case RATE_TYPE_LOCAL:
      return get_field<double>(CURRENT_LOCAL_RATE);
      break;
    default:
      return get_field<double>(CURRENT_DEFAULT_RATE);
  }
}

time_t TrieData::get_current_effective_date(rate_type_t rate_type) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      return get_field<time_t>(CURRENT_INTER_EFFECTIVE_DATE);
      break;
    case RATE_TYPE_INTRA:
      return get_field<time_t>(CURRENT_INTRA_EFFECTIVE_DATE);
      break;
    case RATE_TYPE_LOCAL:
      return get_field<time_t>(CURRENT_LOCAL_EFFECTIVE_DATE);
      break;
    default:
      return get_field<time_t>(CURRENT_DEFAULT_EFFECTIVE_DATE);
  }
}

time_t TrieData::get_current_end_date(rate_type_t rate_type) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      return get_field<time_t>(CURRENT_INTER_END_DATE);
      break;
    case RATE_TYPE_INTRA:
      return get_field<time_t>(CURRENT_INTRA_END_DATE);
      break;
    case RATE_TYPE_LOCAL:
      return get_field<time_t>(CURRENT_LOCAL_END_DATE);
      break;
    default:
      return get_field<time_t>(CURRENT_DEFAULT_END_DATE);
  }
}

void TrieData::set_current_rate(rate_type_t rate_type, double rate) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      set_field<double>(CURRENT_INTER_RATE, rate);
      break;
    case RATE_TYPE_INTRA:
      set_field<double>(CURRENT_INTRA_RATE, rate);
      break;
    case RATE_TYPE_LOCAL:
      set_field<double>(CURRENT_LOCAL_RATE, rate);
      break;
    default:
      set_field<double>(CURRENT_DEFAULT_RATE, rate);
  }
}

void TrieData::set_current_effective_date(rate_type_t rate_type, time_t effective_date) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      set_field<time_t>(CURRENT_INTER_EFFECTIVE_DATE, effective_date);
      break;
    case RATE_TYPE_INTRA:
      set_field<time_t>(CURRENT_INTRA_EFFECTIVE_DATE, effective_date);
      break;
    case RATE_TYPE_LOCAL:
      set_field<time_t>(CURRENT_LOCAL_EFFECTIVE_DATE, effective_date);
      break;
    default:
      set_field<time_t>(CURRENT_DEFAULT_EFFECTIVE_DATE, effective_date);
  }
}

void TrieData::set_current_end_date(rate_type_t rate_type, time_t end_date) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      set_field<time_t>(CURRENT_INTER_END_DATE, end_date);
      break;
    case RATE_TYPE_INTRA:
      set_field<time_t>(CURRENT_INTRA_END_DATE, end_date);
      break;
    case RATE_TYPE_LOCAL:
      set_field<time_t>(CURRENT_LOCAL_END_DATE, end_date);
      break;
    default:
      set_field<time_t>(CURRENT_DEFAULT_END_DATE, end_date);
  }
}

double TrieData::get_future_rate(rate_type_t rate_type) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      return get_field<double>(FUTURE_INTER_RATE);
      break;
    case RATE_TYPE_INTRA:
      return get_field<double>(FUTURE_INTRA_RATE);
      break;
    case RATE_TYPE_LOCAL:
      return get_field<double>(FUTURE_LOCAL_RATE);
      break;
    default:
      return get_field<double>(FUTURE_DEFAULT_RATE);
  }
}

time_t TrieData::get_future_effective_date(rate_type_t rate_type) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      return get_field<time_t>(FUTURE_INTER_EFFECTIVE_DATE);
      break;
    case RATE_TYPE_INTRA:
      return get_field<time_t>(FUTURE_INTRA_EFFECTIVE_DATE);
      break;
    case RATE_TYPE_LOCAL:
      return get_field<time_t>(FUTURE_LOCAL_EFFECTIVE_DATE);
      break;
    default:
      return get_field<time_t>(FUTURE_DEFAULT_EFFECTIVE_DATE);
  }
}

time_t TrieData::get_future_end_date(rate_type_t rate_type) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      return get_field<time_t>(FUTURE_INTER_END_DATE);
      break;
    case RATE_TYPE_INTRA:
      return get_field<time_t>(FUTURE_INTRA_END_DATE);
      break;
    case RATE_TYPE_LOCAL:
      return get_field<time_t>(FUTURE_LOCAL_END_DATE);
      break;
    default:
      return get_field<time_t>(FUTURE_DEFAULT_END_DATE);
  }
}

void TrieData::set_future_rate(rate_type_t rate_type, double rate) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      set_field<double>(FUTURE_INTER_RATE, rate);
      break;
    case RATE_TYPE_INTRA:
      set_field<double>(FUTURE_INTRA_RATE, rate);
      break;
    case RATE_TYPE_LOCAL:
      set_field<double>(FUTURE_LOCAL_RATE, rate);
      break;
    default:
      set_field<double>(FUTURE_DEFAULT_RATE, rate);
  }
}

void TrieData::set_future_effective_date(rate_type_t rate_type, time_t effective_date) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      set_field<time_t>(FUTURE_INTER_EFFECTIVE_DATE, effective_date);
      break;
    case RATE_TYPE_INTRA:
      set_field<time_t>(FUTURE_INTRA_EFFECTIVE_DATE, effective_date);
      break;
    case RATE_TYPE_LOCAL:
      set_field<time_t>(FUTURE_LOCAL_EFFECTIVE_DATE, effective_date);
      break;
    default:
      set_field<time_t>(FUTURE_DEFAULT_EFFECTIVE_DATE, effective_date);
  }
}

void TrieData::set_future_end_date(rate_type_t rate_type, time_t end_date) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      set_field<time_t>(FUTURE_INTER_END_DATE, end_date);
      break;
    case RATE_TYPE_INTRA:
      set_field<time_t>(FUTURE_INTRA_END_DATE, end_date);
      break;
    case RATE_TYPE_LOCAL:
      set_field<time_t>(FUTURE_LOCAL_END_DATE, end_date);
      break;
    default:
      set_field<time_t>(FUTURE_DEFAULT_END_DATE, end_date);
  }
}

int TrieData::get_table_index() {
  return get_field<int>(TABLE_INDEX);
}

void TrieData::set_table_index(size_t table_index) {
  set_field<int>(TABLE_INDEX, table_index);
}

std::string TrieData::get_code_name() {
  if (fields_bitmap & CODE_NAME_ADDRESS)
    return ((ctrl::p_code_pair_t)fields[key_to_field_pos(CODE_NAME_ADDRESS)])->first;
  else
    return "";
}

void TrieData::set_code_name(ctrl::p_code_pair_t code_item) {
  unsigned char field_pos = key_to_field_pos(CODE_NAME_ADDRESS);
  if ((fields_bitmap & CODE_NAME_ADDRESS) == 0) {
    fields_size += sizeof(void*);
    fields = (void**)realloc(fields, fields_size);
    unsigned char count = fields_size / sizeof(void*);
    for (size_t i = count - 1; i > field_pos; --i)
      fields[i] = fields[i-1];
    fields_bitmap |= CODE_NAME_ADDRESS;
  }
  fields[field_pos] = code_item;
}

void TrieData::set_egress_trunk_id(rate_type_t rate_type, unsigned int egress_trunk_id) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      set_field<unsigned int>(INTER_RATE_EGRESS_TRUNK_ID, egress_trunk_id);
      break;
    case RATE_TYPE_INTRA:
      set_field<unsigned int>(INTRA_RATE_EGRESS_TRUNK_ID, egress_trunk_id);
      break;
    case RATE_TYPE_LOCAL:
      set_field<unsigned int>(LOCAL_RATE_EGRESS_TRUNK_ID, egress_trunk_id);
      break;
    default:
      set_field<unsigned int>(DEFAULT_RATE_EGRESS_TRUNK_ID, egress_trunk_id);
  }
}

unsigned int TrieData::get_egress_trunk_id(rate_type_t rate_type) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      return get_field<unsigned int>(INTER_RATE_EGRESS_TRUNK_ID);
      break;
    case RATE_TYPE_INTRA:
      return get_field<unsigned int>(INTRA_RATE_EGRESS_TRUNK_ID);
      break;
    case RATE_TYPE_LOCAL:
      return get_field<unsigned int>(LOCAL_RATE_EGRESS_TRUNK_ID);
      break;
    default:
      return get_field<unsigned int>(DEFAULT_RATE_EGRESS_TRUNK_ID);
  }
}

unsigned int TrieData::get_rate_table_id() {
  return get_field<unsigned int>(RATE_TABLE_ID);
}

void TrieData::set_rate_table_id(unsigned int rate_table_id) {
  set_field<unsigned int>(RATE_TABLE_ID, rate_table_id);
}

Trie::Trie(unsigned int worker_index)
  : worker_index(worker_index),
    children_bitmap(0),
    children_size(0),
    data(new TrieData()),
    children(nullptr) {}

Trie::~Trie() {
  /** IMPORTANT: Individual child disposal is delegated to Controller release queues to be done by tasks/workers **/

  /*size_t count = children_size / sizeof(p_trie_t);
  for (unsigned char i=0; i < count; ++i)
    delete children[i];*/
  free(children);
  delete data;
}

unsigned int Trie::get_worker_index() {
  return worker_index;
}

p_trie_data_t Trie::get_data() {
  return data;
}

void Trie::set_current_data(rate_type_t rate_type, double rate, time_t reference_time, time_t effective_date, time_t end_date, ctrl::p_code_pair_t code_item, unsigned int egress_trunk_id) {
  if ( effective_date <= reference_time && effective_date > data->get_current_effective_date(rate_type) &&
      (end_date <= 0 || end_date >= reference_time)) {
        data->set_current_rate(rate_type, rate);
        data->set_current_effective_date(rate_type, effective_date);
        data->set_current_end_date(rate_type, end_date);
        data->set_code_name(code_item);
        data->set_egress_trunk_id(rate_type, egress_trunk_id);
      if (end_date <= 0) {
        data->set_future_rate(rate_type, rate);
        data->set_future_effective_date(rate_type, reference_time);
        data->set_future_end_date(rate_type, end_date);
      }
      else {
        data->set_future_rate(rate_type, -1);
        data->set_future_effective_date(rate_type, end_date + 1);
        data->set_future_end_date(rate_type, -1);
      }
  }
  if (effective_date > reference_time && (data->get_future_rate(rate_type) == -1 || effective_date < data->get_future_effective_date(rate_type))) {
    data->set_future_rate(rate_type, rate);
    data->set_future_effective_date(rate_type, effective_date);
    data->set_future_end_date(rate_type, end_date);
    if (data->get_current_end_date(rate_type) >= effective_date)
      data->set_current_end_date(rate_type, effective_date - 1);
  }
}

uint16_t Trie::index_to_mask(unsigned char index) {
  if (index < 0 || index > 9)
    throw TrieInvalidChildIndexException();
  return 1 << abs(index - 15);
}

unsigned char Trie::index_to_child_pos(unsigned char index) {
  if (index < 0 || index > 9)
    throw TrieInvalidChildIndexException();
  unsigned char pos = 0;
  unsigned char bits_limit = index;
  uint16_t bitmap = children_bitmap;
  while (bits_limit > 0) {
    if (bitmap & 0x8000) pos++;
    bitmap <<= 1;
    bits_limit--;
  }
  return pos;
}

bool Trie::has_child(unsigned char index) {
  if (index < 0 || index > 9)
    throw TrieInvalidChildIndexException();
  uint16_t mask = index_to_mask(index);
  return children_bitmap & mask;
}

p_trie_t Trie::get_child(unsigned char index) {
  if (index < 0 || index > 9)
    throw TrieInvalidChildIndexException();
  return children[index_to_child_pos(index)];
}

p_trie_t Trie::insert_child(unsigned int worker_index, unsigned char index) {
  if (index < 0 || index > 9)
    throw TrieInvalidChildIndexException();
  children_size += sizeof(p_trie_t);
  children = (p_trie_t*)realloc(children, children_size);
  size_t count = children_size / sizeof(p_trie_t);
  unsigned char child_pos = index_to_child_pos(index);
  for (size_t i = count - 1; i > child_pos; --i)
    children[i] = children[i-1];
  children[child_pos] = new Trie(worker_index);
  uint16_t child_mask = index_to_mask(index);
  children_bitmap |= child_mask;
  return children[child_pos];
}

tbb::mutex* Trie::get_mutex() {
  return &trie_insertion_mutex;
}

/**
    Inserts rate data in the prefix tree at the correct prefix location
*/
void Trie::insert_code(const p_trie_t trie,
                       unsigned int worker_index,
                       unsigned long long code,
                       ctrl::p_code_pair_t code_item,
                       unsigned int rate_table_id,
                       double default_rate,
                       double inter_rate,
                       double intra_rate,
                       double local_rate,
                       time_t effective_date,
                       time_t end_date,
                       time_t reference_time,
                       unsigned int egress_trunk_id) {
  p_trie_data_t root_data = trie->get_data();
  unsigned int root_rate_table_id = root_data->get_rate_table_id();
  if (root_rate_table_id == 0)
    root_data->set_rate_table_id(rate_table_id);
  else if (root_rate_table_id != rate_table_id)
    throw TrieWrongRateTableException();
  p_trie_t current_trie = trie;
  Code dyn_code(code);
  while (dyn_code.has_more_digits()) {
    unsigned char child_index = dyn_code.next_digit();
    if (current_trie->has_child(child_index))
      current_trie = current_trie->get_child(child_index);
    else
      current_trie = current_trie->insert_child(worker_index, child_index);
  }
  for (int i = RATE_TYPE_DEFAULT; i <= RATE_TYPE_LOCAL; i++) {
    rate_type_t rate_type = (rate_type_t)i;
    double rate;
    switch (rate_type) {
      case RATE_TYPE_INTER:
        rate = inter_rate;
        break;
      case RATE_TYPE_INTRA:
        rate = intra_rate;
        break;
      case RATE_TYPE_LOCAL:
        rate = local_rate;
        break;
      case RATE_TYPE_DEFAULT:
        rate = default_rate;
        break;
      default:
        continue;
    }
    if (rate <= 0) continue;  //Don't update rate data if it is inexistent
    current_trie->set_current_data(rate_type, rate, reference_time, effective_date, end_date, code_item, egress_trunk_id);
  }
}

/**
    Longest prefix search implementation... sort of
*/
void Trie::search_code(const p_trie_t trie, unsigned long long code, rate_type_t rate_type, search::SearchResult &search_result, const std::string &filter_code_name, bool include_code) {
  unsigned int rate_table_id = trie->get_data()->get_rate_table_id();
  p_trie_t current_trie = trie;
  unsigned long long code_found = 0, current_code = 0;
  std::string code_name;
  double current_min_rate = -1;
  double current_max_rate = -1;
  double future_min_rate = -1;
  double future_max_rate = -1;
  time_t current_effective_date;
  time_t current_end_date;
  time_t future_effective_date;
  time_t future_end_date;
  unsigned int egress_trunk_id;
  Code dyn_code(code);
  bool children_found = true;
  while (children_found && dyn_code.has_more_digits()) {
    unsigned char child_index = dyn_code.next_digit();
    if (current_trie->has_child(child_index)) {         // If we have a child node, move to it so we can search the longest prefix
      current_trie = current_trie->get_child(child_index);
      current_code =  current_code * 10 + child_index;
      p_trie_data_t data = current_trie->get_data();
      double data_current_rate = data->get_current_rate(rate_type);
      time_t data_current_effective_date = data->get_current_effective_date(rate_type);
      time_t data_current_end_date = data->get_current_end_date(rate_type);
      double data_future_rate = data->get_future_rate(rate_type);
      time_t data_future_effective_date = data->get_future_effective_date(rate_type);
      time_t data_future_end_date = data->get_future_end_date(rate_type);
      if (data_current_rate > 0 ) {
        if (current_min_rate <=0 || data_current_rate < current_min_rate)
          current_min_rate = data_current_rate;
        //if (current_max_rate <=0 || data_current_rate > current_max_rate) {
          code_name = data->get_code_name();
          if (filter_code_name != "" && code_name != filter_code_name)
            continue;
          code_found = current_code;
          current_max_rate = data_current_rate;
          current_effective_date = data_current_effective_date;
          current_end_date = data_current_end_date;
          egress_trunk_id = data->get_egress_trunk_id(rate_type);
          if (future_min_rate <=0 || data_future_rate < future_min_rate)
            future_min_rate = data_future_rate;
          //if (future_max_rate <=0 || data_future_rate > future_max_rate) {
            future_max_rate = data_future_rate;
            future_effective_date = data_future_effective_date;
            future_end_date = data_future_end_date;
          //}
        //}
      }
    }
    else
      children_found = false;
  }
  if (code_found) {
    search_result.insert(include_code ? code_found : 0,
                         code_name,
                         rate_table_id,
                         rate_type,
                         current_min_rate,
                         current_max_rate,
                         future_min_rate,
                         future_max_rate,
                         current_effective_date,
                         current_end_date,
                         future_effective_date,
                         future_end_date,
                         egress_trunk_id);
  }
}

/**
    Search a prefix tree in pre-order
*/
/*void Trie::total_search_code(const p_trie_t trie, rate_type_t rate_type, search::SearchResult &search_result) {
  search_nodes_t nodes;  //Use vector as a stack, to be able to access ancestor elements to know the rate code
  nodes.emplace_back(new search_node_t({0, trie}));
  unsigned long long code_found = 0;
  std::string code_name;
  double current_min_rate = -1;
  double current_max_rate = -1;
  double future_min_rate = -1;
  double future_max_rate = -1;
  time_t effective_date;
  time_t end_date;
  time_t future_effective_date;
  time_t future_end_date;
  p_trie_t current_trie;
  unsigned char child_index = 1;
  unsigned int rate_table_id = trie->get_data()->get_rate_table_id();
  while (child_index < 10 && !trie->has_child(child_index))
    child_index++;
  if (child_index == 10)
    return;
  else
    current_trie = trie->get_child(child_index);
  while (!nodes.empty()) {
    ** Update the searched variables from the current (parent) prefix tree
    nodes.emplace_back(new search_node_t({child_index, current_trie}));
    total_search_update_vars(current_trie, nodes, code_found, code_name, rate_type,
                             current_min_rate, current_max_rate, future_min_rate, future_max_rate,
                             effective_date, end_date, future_effective_date, future_end_date);


    ** Continue the search algorithm visiting the left-most child
    while (current_trie->has_child(0)) {
      current_trie = current_trie->get_child(0);
      ** Update the searched variables from the current (left-most child) prefix tree
      nodes.emplace_back(new search_node_t({0, current_trie}));
      total_search_update_vars(current_trie, nodes, code_found, code_name, rate_type,
                               effective_date, end_date, future_effective_date, future_end_date);
                               current_min_rate, current_max_rate, future_min_rate, future_max_rate,
    }

    ** Continue the search algorithm to the right: selecting a sibling or uncle prefix tree node
    p_search_node_t last_node = nodes.back();
    p_trie_t prev_trie = current_trie;
    while (!nodes.empty() && prev_trie == current_trie) {
      if (child_index == 9) {
        child_index = last_node->child_index;
        nodes.pop_back();
        last_node = nodes.back();
      }
      else if (last_node->trie->has_child(++child_index))
        current_trie = last_node->trie->get_child(child_index);
    }
  }
  if (code_found)
    search_result.insert(code_found,
                         code_name,
                         rate_table_id,
                         rate_type,
                         current_min_rate,
                         current_max_rate,
                         future_min_rate,
                         future_max_rate,
                         effective_date,
                         end_date,
                         future_effective_date,
                         future_end_date);
  for (size_t i = 0; i < nodes.size(); ++i)
    delete nodes[i];
  nodes.clear();
}*/

void Trie::total_search_update_vars(const p_trie_t &current_trie,
                                    const search_nodes_t &nodes,
                                    unsigned long long &code,
                                    std::string &code_name,
                                    const rate_type_t rate_type,
                                    double &current_min_rate,
                                    double &current_max_rate,
                                    double &future_min_rate,
                                    double &future_max_rate,
                                    time_t &current_effective_date,
                                    time_t &current_end_date,
                                    time_t &future_effective_date,
                                    time_t &future_end_date) {
  p_trie_data_t data = current_trie->get_data();
  double data_current_rate = data->get_current_rate(rate_type);
  time_t data_current_effective_date = data->get_current_effective_date(rate_type);
  time_t data_current_end_date = data->get_current_end_date(rate_type);
  double data_future_rate = data->get_future_rate(rate_type);
  time_t data_future_effective_date = data->get_future_effective_date(rate_type);
  time_t data_future_end_date = data->get_future_end_date(rate_type);
  if (data_current_rate > 0) {
    if (current_min_rate <=  0 || data_current_rate < current_min_rate)
      current_min_rate = data_current_rate;
    if (current_max_rate <=  0 || data_current_rate > current_max_rate) {
      code = 0;
      for (auto it = nodes.begin(); it != nodes.end(); ++it)
        code = code * 10 + (*it)->child_index;
      current_max_rate = data_current_rate;
      current_effective_date = data_current_effective_date;
      current_end_date = data_current_end_date;
      code_name = data->get_code_name();
    }
    if (future_min_rate <=  0 || data_future_rate < future_min_rate)
      future_min_rate = data_future_rate;
    if (future_max_rate <=  0 || data_future_rate > future_max_rate) {
      future_max_rate = data_future_rate;
      future_effective_date = data_future_effective_date;
      future_end_date = data_future_end_date;
    }
  }
}

/*void Trie::insert_table_index(const p_trie_t trie, unsigned int worker_index, unsigned int rate_table_id, size_t index) {
  p_trie_t current_trie = trie;
  Code dyn_rate_table_id(rate_table_id);
  while (dyn_rate_table_id.has_more_digits()) {
    unsigned char child_index = dyn_rate_table_id.next_digit();
    if (current_trie->has_child(child_index))
      current_trie = current_trie->get_child(child_index);
    else
      current_trie = current_trie->insert_child(worker_index, child_index);
  }
  p_trie_data_t trie_data = current_trie->get_data();
  trie_data->set_table_index(index);
}

int Trie::search_table_index(const p_trie_t trie, unsigned int rate_table_id) {
  p_trie_t current_trie = trie;
  Code dyn_rate_table_id(rate_table_id);
  bool children_found = true;
  while (children_found && dyn_rate_table_id.has_more_digits()) {
    unsigned char child_index = dyn_rate_table_id.next_digit();
    if (current_trie->has_child(child_index))
      current_trie = current_trie->get_child(child_index);
    else
      children_found = false;
  }
  if (children_found) {
    p_trie_data_t trie_data = current_trie->get_data();
    return trie_data->get_table_index();
  }
  else
    return -1;
}
*/
