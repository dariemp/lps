#include "trie.hxx"
#include "exceptions.hxx"
#include <algorithm>
#include <iostream>

using namespace trie;

TrieData::TrieData() :rate(-1), effective_date(-1), end_date(-1) {}

double TrieData::get_rate() {
  return rate;
}

time_t TrieData::get_effective_date() {
  return effective_date;
}

time_t TrieData::get_end_date() {
  return end_date;
}

void TrieData::set_rate(double rate) {
  this->rate = rate;
}

void TrieData::set_effective_date(time_t effective_date) {
  this->effective_date = effective_date;
}

void TrieData::set_end_date(time_t end_date) {
  this->end_date = end_date;
}

Trie::Trie() {
  for (int i=0; i < 10; ++i) {
    //children.push_back(nullptr);
    children[i].reset(nullptr);
  }
}

p_trie_data_t Trie::get_data() {
  return &data;
}

void Trie::set_data(double rate, time_t effective_date, time_t end_date) {
  data.set_rate(rate);
  data.set_effective_date(effective_date);
  data.set_end_date(end_date);
}

p_trie_t Trie::get_child(unsigned char index) {
  if (index < 0 || index > 9)
    throw TrieInvalidChildIndexException();
  if (children[index].get() == nullptr)
      children[index].reset(new Trie());  // Make the prefix tree grow, someone needs more children
  return children[index].get();
}

bool Trie::has_child(unsigned char index) {
  if (index < 0 || index > 9)
    throw TrieInvalidChildIndexException();
  return children[index].get() != nullptr;
}

/**
    Inserts rate data in the prefix tree at the correct prefix location
*/
void Trie::insert(p_trie_t trie, const char *prefix, size_t prefix_length, double rate, time_t effective_date, time_t end_date) {
  tbb::mutex::scoped_lock lock(trie_insertion_mutex);  // One thread at a time, please
  if (prefix_length < 0)
    throw TrieInvalidInsertionException();
  while (prefix_length > 0) {
    unsigned char child_index = prefix[0] - 48; // convert "0", "1", "2"... to 0, 1, 2,...
    trie = trie->get_child(child_index);
    prefix++;
    prefix_length--;
  }
  p_trie_data_t trie_data = trie->get_data();
  double old_rate = trie_data->get_rate();
  time_t old_effective_date = trie_data->get_effective_date();
  time_t old_end_date = trie_data->get_end_date();

  /******* SOLUTION THAT DOES NOT RESOLVE CONFLICTING RATES ******/
  /*try {
    if (old_rate == new_data->get_rate()) {
      if (trie_data->get_effective_date() != new_data->get_effective_date() ||
          trie_data->get_end_date() != new_data->get_end_date())
        throw TrieCollisionSameRateException();
    }
    else if (old_rate != -1)
      throw TrieCollisionDifferentRateException();
    else
      trie->set_data(new_data);
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "Conflicting prefix: " << original_prefix << std::endl;
    exit(EXIT_FAILURE);
  }*/

  /****** SOLUTION THAT SOLVES CONFLICTING RATES ******/
  if ( old_rate == -1 || (old_end_date == -1 &&  end_date != -1) ||
      (old_end_date != -1 &&  end_date != -1 && effective_date > old_effective_date))
    trie->set_data(rate, effective_date, end_date);
}

/**
    Longest prefix search implementation... sort of
*/
void Trie::search(p_trie_t trie, const char *prefix, size_t prefix_length, unsigned int rate_table_id, ctrl::p_code_names_t code_names, search::SearchResult &search_result) {
  if (prefix_length < 0)
    throw TrieInvalidInsertionException();
  unsigned long long code = 0, current_code = 0;
  std::string code_name;
  double current_min_rate = -1;
  double current_max_rate = -1;
  double future_min_rate = -1;
  double future_max_rate = -1;
  time_t effective_date;
  time_t end_date;
  time_t future_effective_date;
  time_t future_end_date;
  while (prefix_length > 0) {
    unsigned char child_index = prefix[0] - 48; // convert "0", "1", "2"... to 0, 1, 2,...
    //std::cout << "debug: child index to go: " << (char)(child_index + 48) << std::endl;
    if (trie->has_child(child_index)) {                // If we have a child node, move to it so we can search the longest prefix
      trie = trie->get_child(child_index);
      current_code =  current_code * 10 + child_index;
      p_trie_data_t data = trie->get_data();
      double data_rate = data->get_rate();
      time_t data_effective_date = data->get_effective_date();
      time_t data_end_date = data->get_end_date();
      if (data_rate != -1) {
        if (data_end_date == -1) {
          if (future_min_rate == -1 || data_rate < future_min_rate)
            future_min_rate = data_rate;
          if (future_max_rate == -1 || data_rate > future_max_rate) {
            future_max_rate = data_rate;
            future_effective_date = data_effective_date;
            future_end_date = data_end_date;
          }
        }
        else {
          if (current_min_rate == -1 || data_rate < current_min_rate)
            current_min_rate = data_rate;
          if (current_max_rate == -1 || data_rate > current_max_rate) {
            code = current_code;
            current_max_rate = data_rate;
            effective_date = data_effective_date;
            end_date = data_end_date;
          }
        }
      }
      prefix++;
      prefix_length--;
    }
    else
      prefix_length = 0;      // Trick to stop the loop if we are in a leaf, longest prefix found
  }
  /*std::cout << "debug: current prefix: " << prefix << std::endl;
  std::cout << "debug: current rate: " << trie->get_data()->get_rate() << std::endl;
  std::cout << "debug: longest_prefix_rate: " << longest_prefix_rate << std::endl;*/
  if (code)
    search_result.insert(code,
                         (*code_names)[code],
                         rate_table_id,
                         current_min_rate,
                         current_max_rate,
                         future_min_rate,
                         future_max_rate,
                         effective_date,
                         end_date,
                         future_effective_date,
                         future_end_date);
}

/**
    Search a prefix tree in pre-order
*/
void Trie::total_search(p_trie_t trie, unsigned int rate_table_id, ctrl::p_code_names_t code_names, search::SearchResult &search_result) {
  std::cout << "total search" << std::endl;
  search_nodes_t nodes;
  nodes.emplace_back(new search_node_t({0, trie}));
  unsigned long long code = 0;
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
  std::cout << "top level children search" << std::endl;
  //while (child_index < 10 && trie->get_child(child_index)->get_data()->get_rate() == -1)
  while (child_index < 10 && trie->has_child(child_index))
    child_index++;
  if (child_index == 10)
    return;
  else
    current_trie = trie->get_child(child_index);
  std::cout << "start search" << std::endl;
  while (!nodes.empty()) {
    /** Update the searched variables from the current (parent) trie **/
    nodes.emplace_back(new search_node_t({child_index, current_trie}));
    total_search_update_vars(current_trie, nodes, code,
                             current_min_rate, current_max_rate, future_min_rate, future_max_rate,
                             effective_date, end_date, future_effective_date, future_end_date);


    /** Continue the search algorithm visiting the left-most child **/
    /*p_trie_t current_trie = current_trie->get_child(0);
    while (current_trie->get_data()->get_rate() != -1) {*/
    while (current_trie->has_child(0)) {
      std::cout << "left-most" << std::endl;
      current_trie = current_trie->get_child(0);
      /** Update the searched variables from the current (left-most child) trie **/
      nodes.emplace_back(new search_node_t({0, current_trie}));
      total_search_update_vars(current_trie, nodes, code,
                               current_min_rate, current_max_rate, future_min_rate, future_max_rate,
                               effective_date, end_date, future_effective_date, future_end_date);

      //current_trie = current_trie->get_child(0);
    }

    /** Continue the search algorithm to the right: selecting a new trie node **/
    p_search_node_t last_node = nodes.back().get();
    p_trie_t prev_trie = current_trie;
    while (!nodes.empty() && prev_trie == current_trie) {
      if (child_index == 9) {
        std::cout << "end of children" << std::endl;
        child_index = last_node->child_index;
        nodes.pop_back();
        last_node = nodes.back().get();
      }
      else if (last_node->trie->has_child(++child_index)) {
        std::cout << "right move" << std::endl;
        current_trie = last_node->trie->get_child(child_index);
      }
    }
  }
  if (code)
    search_result.insert(code,
                         (*code_names)[code],
                         rate_table_id,
                         current_min_rate,
                         current_max_rate,
                         future_min_rate,
                         future_max_rate,
                         effective_date,
                         end_date,
                         future_effective_date,
                         future_end_date);
}

void Trie::total_search_update_vars(const p_trie_t &current_trie,
                                    const search_nodes_t &nodes,
                                    unsigned long long &code,
                                    double &current_min_rate,
                                    double &current_max_rate,
                                    double &future_min_rate,
                                    double &future_max_rate,
                                    time_t &effective_date,
                                    time_t &end_date,
                                    time_t &future_effective_date,
                                    time_t &future_end_date) {
  p_trie_data_t data = current_trie->get_data();
  double data_rate = data->get_rate();
  time_t data_effective_date = data->get_effective_date();
  time_t data_end_date = data->get_end_date();
  if (data_rate != -1) {
    if (data_end_date == -1) {
      if (future_min_rate == -1 || data_rate < future_min_rate)
        future_min_rate = data_rate;
      if (future_max_rate == -1 || data_rate > future_max_rate) {
        future_max_rate = data_rate;
        future_effective_date = data_effective_date;
        future_end_date = data_end_date;
      }
    }
    else {
      if (current_min_rate == -1 || data_rate < current_min_rate)
        current_min_rate = data_rate;
      if (current_max_rate == -1 || data_rate > current_max_rate) {
        code = 0;
        for (auto it = nodes.begin(); it != nodes.end(); ++it)
          code = code * 10 + (*it)->child_index;
        current_max_rate = data_rate;
        effective_date = data_effective_date;
        end_date = data_end_date;
      }
    }
  }
}
