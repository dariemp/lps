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
    Longest prefix search implementation modified to priorize more recent rate events
*/
p_trie_data_t Trie::search(p_trie_t trie, const char *prefix, size_t prefix_length) {
  if (prefix_length < 0)
    throw TrieInvalidInsertionException();
  p_trie_data_t best_time_data = nullptr;
  p_trie_data_t longest_prefix_data = nullptr;
  while (prefix_length > 0) {
    unsigned char child_index = prefix[0] - 48; // convert "0", "1", "2"... to 0, 1, 2,...
    //std::cout << "debug: child index to go: " << (char)(child_index + 48) << std::endl;
    if (trie->has_child(child_index)) {                // If we have a child node, move to it so we can search the longest prefix
      trie = trie->get_child(child_index);
      p_trie_data_t data = trie->get_data();
      double rate = data->get_rate();
      time_t effective_date = data->get_effective_date();
      if (rate != -1) {
        longest_prefix_data = data;
        if (data->get_end_date() != -1 &&   // Only save this date if the rate event has finished
           (!best_time_data || effective_date > best_time_data->get_effective_date())) {
           best_time_data = data;                    // Save the nearest date to now, since time dimension is important
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

  if (best_time_data)
    return best_time_data;     // We have a rate with a shorter prefix, but a more recent effective_date and has an end_date
  else if (longest_prefix_data)
    return longest_prefix_data;   // We have the rate with the longest prefix
  else
    return nullptr;           // No rate data found in the current prefix tree for the given prefix;
}
