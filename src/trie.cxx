#include "trie.hxx"
#include "exceptions.hxx"
#include <algorithm>
#include <iostream>

using namespace trie;

TrieData::TrieData() {
  fields = new trie_data_fields_t();
}

TrieData::~TrieData() {
  for (auto it = fields->begin(); it != fields->end(); ++it)
    if (it->first != CODE_NAME_ADDRESS)
      free(it->second);
  delete fields;
}

double TrieData::get_double_field(trie_data_field_t key) {
  if (fields->find(key) == fields->end())
    return -1;
  else
    return *(double*)(*fields)[key];
}

time_t TrieData::get_time_field(trie_data_field_t key) {
  if (fields->find(key) == fields->end())
    return -1;
  else
    return *(time_t*)(*fields)[key];
}

void TrieData::set_double_field(trie_data_field_t key, double value) {
  if (value <= 0)
    return;
  if (fields->find(key) == fields->end())
    (*fields)[key] = malloc(sizeof(double));
  *(double*)(*fields)[key] = value;
}

void TrieData::set_time_field(trie_data_field_t key, time_t value) {
  if (value <= 0)
    return;
  if (fields->find(key) == fields->end())
    (*fields)[key] = malloc(sizeof(time_t));
  *(time_t*)(*fields)[key] = value;
}

double TrieData::get_current_rate(rate_type_t rate_type) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      return get_double_field(CURRENT_INTER_RATE);
      break;
    case RATE_TYPE_INTRA:
      return get_double_field(CURRENT_INTRA_RATE);
      break;
    case RATE_TYPE_LOCAL:
      return get_double_field(CURRENT_LOCAL_RATE);
      break;
    default:
      return get_double_field(CURRENT_DEFAULT_RATE);
  }
}

time_t TrieData::get_current_effective_date(rate_type_t rate_type) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      return get_time_field(CURRENT_INTER_EFFECTIVE_DATE);
      break;
    case RATE_TYPE_INTRA:
      return get_time_field(CURRENT_INTRA_EFFECTIVE_DATE);
      break;
    case RATE_TYPE_LOCAL:
      return get_time_field(CURRENT_LOCAL_EFFECTIVE_DATE);
      break;
    default:
      return get_time_field(CURRENT_DEFAULT_EFFECTIVE_DATE);
  }
}

time_t TrieData::get_current_end_date(rate_type_t rate_type) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      return get_time_field(CURRENT_INTER_END_DATE);
      break;
    case RATE_TYPE_INTRA:
      return get_time_field(CURRENT_INTRA_END_DATE);
      break;
    case RATE_TYPE_LOCAL:
      return get_time_field(CURRENT_LOCAL_END_DATE);
      break;
    default:
      return get_time_field(CURRENT_DEFAULT_END_DATE);
  }
}

void TrieData::set_current_rate(rate_type_t rate_type, double rate) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      set_double_field(CURRENT_INTER_RATE, rate);
      break;
    case RATE_TYPE_INTRA:
      set_double_field(CURRENT_INTRA_RATE, rate);
      break;
    case RATE_TYPE_LOCAL:
      set_double_field(CURRENT_LOCAL_RATE, rate);
      break;
    default:
      set_double_field(CURRENT_DEFAULT_RATE, rate);
  }
}

void TrieData::set_current_effective_date(rate_type_t rate_type, time_t effective_date) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      set_time_field(CURRENT_INTER_EFFECTIVE_DATE, effective_date);
      break;
    case RATE_TYPE_INTRA:
      set_time_field(CURRENT_INTRA_EFFECTIVE_DATE, effective_date);
      break;
    case RATE_TYPE_LOCAL:
      set_time_field(CURRENT_LOCAL_EFFECTIVE_DATE, effective_date);
      break;
    default:
      set_time_field(CURRENT_DEFAULT_EFFECTIVE_DATE, effective_date);
  }
}

void TrieData::set_current_end_date(rate_type_t rate_type, time_t end_date) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      set_time_field(CURRENT_INTER_END_DATE, end_date);
      break;
    case RATE_TYPE_INTRA:
      set_time_field(CURRENT_INTRA_END_DATE, end_date);
      break;
    case RATE_TYPE_LOCAL:
      set_time_field(CURRENT_LOCAL_END_DATE, end_date);
      break;
    default:
      set_time_field(CURRENT_DEFAULT_END_DATE, end_date);
  }
}



double TrieData::get_future_rate(rate_type_t rate_type) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      return get_double_field(FUTURE_INTER_RATE);
      break;
    case RATE_TYPE_INTRA:
      return get_double_field(FUTURE_INTRA_RATE);
      break;
    case RATE_TYPE_LOCAL:
      return get_double_field(FUTURE_LOCAL_RATE);
      break;
    default:
      return get_double_field(FUTURE_DEFAULT_RATE);
  }
}

time_t TrieData::get_future_effective_date(rate_type_t rate_type) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      return get_time_field(FUTURE_INTER_EFFECTIVE_DATE);
      break;
    case RATE_TYPE_INTRA:
      return get_time_field(FUTURE_INTRA_EFFECTIVE_DATE);
      break;
    case RATE_TYPE_LOCAL:
      return get_time_field(FUTURE_LOCAL_EFFECTIVE_DATE);
      break;
    default:
      return get_time_field(FUTURE_DEFAULT_EFFECTIVE_DATE);
  }
}

time_t TrieData::get_future_end_date(rate_type_t rate_type) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      return get_time_field(FUTURE_INTER_END_DATE);
      break;
    case RATE_TYPE_INTRA:
      return get_time_field(FUTURE_INTRA_END_DATE);
      break;
    case RATE_TYPE_LOCAL:
      return get_time_field(FUTURE_LOCAL_END_DATE);
      break;
    default:
      return get_time_field(FUTURE_DEFAULT_END_DATE);
  }
}

void TrieData::set_future_rate(rate_type_t rate_type, double rate) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      set_double_field(FUTURE_INTER_RATE, rate);
      break;
    case RATE_TYPE_INTRA:
      set_double_field(FUTURE_INTRA_RATE, rate);
      break;
    case RATE_TYPE_LOCAL:
      set_double_field(FUTURE_LOCAL_RATE, rate);
      break;
    default:
      set_double_field(FUTURE_DEFAULT_RATE, rate);
  }
}

void TrieData::set_future_effective_date(rate_type_t rate_type, time_t effective_date) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      set_double_field(FUTURE_INTER_EFFECTIVE_DATE, effective_date);
      break;
    case RATE_TYPE_INTRA:
      set_double_field(FUTURE_INTRA_EFFECTIVE_DATE, effective_date);
      break;
    case RATE_TYPE_LOCAL:
      set_double_field(FUTURE_LOCAL_EFFECTIVE_DATE, effective_date);
      break;
    default:
      set_double_field(FUTURE_DEFAULT_EFFECTIVE_DATE, effective_date);
  }
}

void TrieData::set_future_end_date(rate_type_t rate_type, time_t end_date) {
  switch (rate_type) {
    case RATE_TYPE_INTER:
      set_double_field(FUTURE_INTER_END_DATE, end_date);
      break;
    case RATE_TYPE_INTRA:
      set_double_field(FUTURE_INTRA_END_DATE, end_date);
      break;
    case RATE_TYPE_LOCAL:
      set_double_field(FUTURE_LOCAL_END_DATE, end_date);
      break;
    default:
      set_double_field(FUTURE_DEFAULT_END_DATE, end_date);
  }
}

unsigned long long TrieData::get_table_index() {
  if (fields->find(TABLE_INDEX) == fields->end())
    return 0;
  else
    return *(unsigned long long*)(*fields)[TABLE_INDEX];
}

void TrieData::set_table_index(unsigned long long table_index) {
  if (table_index == 0)
    return;
  if (fields->find(TABLE_INDEX) == fields->end())
    (*fields)[TABLE_INDEX] = malloc(sizeof(unsigned long long));
  *(unsigned long long*)(*fields)[TABLE_INDEX] = table_index;
}

std::string TrieData::get_code_name() {
  ctrl::p_code_pair_t p_code_bucket = (ctrl::p_code_pair_t)(*fields)[CODE_NAME_ADDRESS];
  return p_code_bucket->first;
}

void TrieData::set_code_name_ptr(ctrl::p_code_pair_t code_name_ptr) {
  (*fields)[CODE_NAME_ADDRESS] = code_name_ptr;
}

unsigned int TrieData::get_rate_table_id() {
  if (fields->find(TABLE_RATE_ID) == fields->end())
    return 0;
  else
    return *(unsigned int*)(*fields)[TABLE_RATE_ID];
}

void TrieData::set_rate_table_id(unsigned int rate_table_id) {
  if (rate_table_id == 0)
    return;
  if (fields->find(TABLE_RATE_ID) == fields->end())
    (*fields)[TABLE_RATE_ID] = malloc(sizeof(unsigned int));
  *(unsigned int*)(*fields)[TABLE_RATE_ID] = rate_table_id;
}

Trie::Trie() {
  data = new TrieData();
  for (unsigned char i=0; i < 10; ++i)
    children[i] = nullptr;
}

Trie::~Trie() {
  for (unsigned char i=0; i < 10; ++i)
    delete children[i];
  delete data;
}


p_trie_data_t Trie::get_data() {
  return data;
}

void Trie::set_current_data(rate_type_t rate_type, double rate, time_t effective_date, time_t end_date, ctrl::p_code_pair_t code_name_ptr) {
  data->set_current_rate(rate_type, rate);
  data->set_current_effective_date(rate_type, effective_date);
  data->set_current_end_date(rate_type, end_date);
  data->set_code_name_ptr(code_name_ptr);
}

void Trie::set_future_data(rate_type_t rate_type, double rate, time_t effective_date, time_t end_date, ctrl::p_code_pair_t code_name_ptr) {
  data->set_future_rate(rate_type, rate);
  data->set_future_effective_date(rate_type, effective_date);
  data->set_future_end_date(rate_type, end_date);
  data->set_code_name_ptr(code_name_ptr);
}

bool Trie::has_child(unsigned char index) {
  if (index < 0 || index > 9)
    throw TrieInvalidChildIndexException();
  return (bool)children[index];
}

p_trie_t Trie::get_child(unsigned char index) {
  if (index < 0 || index > 9)
    throw TrieInvalidChildIndexException();
  return children[index];
}

p_trie_t Trie::insert_child(unsigned char index) {
  if (index < 0 || index > 9)
    throw TrieInvalidChildIndexException();
  children[index] = new Trie();
  return children[index];
}

/**
    Inserts rate data in the prefix tree at the correct prefix location
*/
void Trie::insert_code(const p_trie_t trie,
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
                       time_t reference_time) {
  tbb::mutex::scoped_lock lock(trie_insertion_mutex);  // One thread at a time, please
  if (code_length <= 0)
    throw TrieInvalidPrefixLengthException();
  p_trie_data_t root_data = trie->get_data();
  unsigned int root_rate_table_id = root_data->get_rate_table_id();
  if (root_rate_table_id == 0)
    root_data->set_rate_table_id(rate_table_id);
  else if (root_rate_table_id != rate_table_id)
    throw TrieWrongRateTableException();
  p_trie_t current_trie = trie;
  while (code_length > 0) {
    unsigned char child_index = code[0] - 48; // convert "0", "1", "2"... to 0, 1, 2,...
    if (current_trie->has_child(child_index))
      current_trie = current_trie->get_child(child_index);
    else
      current_trie = current_trie->insert_child(child_index);
    code++;
    code_length--;
  }
  p_trie_data_t trie_data = current_trie->get_data();
  for (int i = RATE_TYPE_DEFAULT; i <= RATE_TYPE_LOCAL; i++) {
    rate_type_t rate_type = (rate_type_t)i;
    time_t old_current_effective_date = trie_data->get_current_effective_date(rate_type);
    time_t old_future_effective_date = trie_data->get_future_effective_date(rate_type);
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
    if (rate == -1) continue;  //Don't update rate data if it is inexistent
    if ( effective_date <= reference_time && effective_date > old_current_effective_date &&
        (end_date == -1 || end_date >= reference_time)) {
      current_trie->set_current_data(rate_type, rate, effective_date, end_date, code_name_ptr);
    }
    if ( effective_date > reference_time &&
        (old_future_effective_date == -1 || effective_date < old_future_effective_date))
      current_trie->set_future_data(rate_type, rate, effective_date, end_date, code_name_ptr);
  }
}

/**
    Longest prefix search implementation... sort of
*/
void Trie::search_code(const p_trie_t trie, const char *code, size_t code_length, rate_type_t rate_type, search::SearchResult &search_result) {
  if (code_length <= 0)
    throw TrieInvalidPrefixLengthException();
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
  while (code_length > 0) {
    unsigned char child_index = code[0] - 48; // convert "0", "1", "2"... to 0, 1, 2,...
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
      if (data_current_rate != -1) {
        if (current_min_rate == -1 || data_current_rate < current_min_rate)
          current_min_rate = data_current_rate;
        if (current_max_rate == -1 || data_current_rate > current_max_rate) {
          code_found = current_code;
          current_max_rate = data_current_rate;
          current_effective_date = data_current_effective_date;
          current_end_date = data_current_end_date;
          code_name = data->get_code_name();
        }
        if (future_min_rate == -1 || data_future_rate < future_min_rate)
          future_min_rate = data_future_rate;
        if (future_max_rate == -1 || data_future_rate > future_max_rate) {
          future_max_rate = data_future_rate;
          future_effective_date = data_future_effective_date;
          future_end_date = data_future_end_date;
        }
      }
      code++;
      code_length--;
    }
    else
      code_length = 0;      // Trick to stop the loop if we are in a leaf, longest prefix found
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
                         current_effective_date,
                         current_end_date,
                         future_effective_date,
                         future_end_date);
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
  if (data_current_rate != -1) {
    if (current_min_rate == -1 || data_current_rate < current_min_rate)
      current_min_rate = data_current_rate;
    if (current_max_rate == -1 || data_current_rate > current_max_rate) {
      code = 0;
      for (auto it = nodes.begin(); it != nodes.end(); ++it)
        code = code * 10 + (*it)->child_index;
      current_max_rate = data_current_rate;
      current_effective_date = data_current_effective_date;
      current_end_date = data_current_end_date;
      code_name = data->get_code_name();
    }
    if (future_min_rate == -1 || data_future_rate < future_min_rate)
      future_min_rate = data_future_rate;
    if (future_max_rate == -1 || data_future_rate > future_max_rate) {
      future_max_rate = data_future_rate;
      future_effective_date = data_future_effective_date;
      future_end_date = data_future_end_date;
    }
  }
}

void Trie::insert_table_index(const p_trie_t trie, const char *prefix, size_t prefix_length, unsigned long long index) {
  tbb::mutex::scoped_lock lock(trie_insertion_mutex);  // One thread at a time, please
  if (prefix_length <= 0)
    throw TrieInvalidPrefixLengthException();
  p_trie_t current_trie = trie;
  while (prefix_length > 0) {
    unsigned char child_index = prefix[0] - 48; // convert "0", "1", "2"... to 0, 1, 2,...
    if (current_trie->has_child(child_index))
      current_trie = current_trie->get_child(child_index);
    else
      current_trie = current_trie->insert_child(child_index);
    prefix++;
    prefix_length--;
  }
  p_trie_data_t trie_data = current_trie->get_data();
  trie_data->set_table_index(index);
}

unsigned long long Trie::search_table_index(const p_trie_t trie, const char *prefix, size_t prefix_length) {
  if (prefix_length <= 0)
    throw TrieInvalidPrefixLengthException();
  p_trie_t current_trie = trie;
  bool has_child = true;
  while (has_child && prefix_length > 0) {
    unsigned char child_index = prefix[0] - 48; // convert "0", "1", "2"... to 0, 1, 2,...
    if (current_trie->has_child(child_index)) {         // If we have a child node, move to it so we can search the longest prefix
      current_trie = current_trie->get_child(child_index);
      prefix++;
      prefix_length--;
    }
    else
      has_child = false;
  }
  if (prefix_length == 0) {
    p_trie_data_t trie_data = current_trie->get_data();
    return trie_data->get_table_index();
  }
  else
    return 0;
}
