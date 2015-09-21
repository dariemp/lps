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

Trie::Trie(): data(std::unique_ptr<TrieData>(new TrieData())) {
  for (int i=0; i< 10; ++i)
    children[i] = nullptr;
}

p_trie_data_t Trie::get_data() {
  return data.get();
}

void Trie::set_data(p_trie_data_t new_data) {
  this->data.reset(new_data);
}

static unsigned int count = 0;

p_trie_t Trie::get_child(unsigned char index) {
  if (index < 0 || index > 9) {
    count++;
    std::cerr << "Bad character: " << index + 48 << ", count: " << count << std::endl;
    throw TrieInvalidChildIndexException();
  }
  if (!children[index])
    for (int i=0; i< 10; ++i)
      children[i] = uptr_trie_t(new Trie());  // Make the prefix tree grow, someone needs more children
  return children[index].get();
}

bool Trie::has_child_data(unsigned char index) {
  if (index < 0 || index > 9)
    throw TrieInvalidChildIndexException();
  return children[index]->get_data()->get_rate() != -1;
}

/**
    Inserts rate data in the prefix tree at the correct prefix location
*/
void Trie::insert(p_trie_t trie, const char *prefix, size_t prefix_length, p_trie_data_t new_data) {
  tbb::mutex::scoped_lock lock(trie_insertion_mutex);  // Order! order! one thread at a time, please!
  if (prefix_length < 0)
    throw TrieInvalidInsertionException();
  while (prefix_length > 0) {
    unsigned char child_index = (unsigned char)prefix[0] - 48; // convert "0", "1", "2"... to 0, 1, 2,...
    trie = trie->get_child(child_index);
    prefix++;
    prefix_length--;
  }
  p_trie_data_t trie_data = trie->get_data();
  double old_rate = trie_data->get_rate();
  if (old_rate == new_data->get_rate()) {
    if (trie_data->get_effective_date() != new_data->get_effective_date() ||
        trie_data->get_end_date() != new_data->get_end_date())
      throw TrieCollisionException();
  }
  else if (old_rate != -1)
    throw TrieCollisionException();
  else
    trie->set_data(new_data);
}

/**
    Longest prefix search implementation modified to priorize more recent rate events
*/
p_trie_data_t Trie::search(p_trie_t trie, const char *prefix, size_t prefix_length) {
  if (prefix_length < 0)
    throw TrieInvalidInsertionException();
  p_trie_data_t best_time_data = nullptr;
  while (prefix_length > 0) {
    p_trie_data_t data = trie->get_data();
    if (data && data->get_end_date() != -1 &&   // Only save this date if the rate event has finished
        (!best_time_data || data->get_effective_date() > best_time_data->get_effective_date()))
      best_time_data = data;                    // Save the nearest date to now, since time dimension is important
    unsigned char child_index = (unsigned char)prefix[0] - 48; // convert "0", "1", "2"... to 0, 1, 2,...
    if (trie->has_child_data(child_index)) {    // If we have a child node with data, move to it so we can search the longest prefix
      prefix++;
      prefix_length--;
      trie = trie->get_child(child_index);
    }
    else
      prefix_length = 0;      // Trick to stop the loop if we are in a leaf, longest prefix found
  }
  if (best_time_data)
    return best_time_data;     // We have a rate with a shorter prefix, but a more recent effective_date and has an end_date
  else
    return trie->get_data();   // We have the rate with the longest prefix
}
