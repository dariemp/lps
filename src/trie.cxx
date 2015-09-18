#include "trie.hxx"
#include "exceptions.hxx"
#include <algorithm>

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

Trie::Trie(): data(std::make_shared<TrieData>()) {
  fill_n(children, 10, nullptr);
}

std::shared_ptr<TrieData> Trie::get_data() {
  return data;
}

void Trie::set_data(std::shared_ptr<TrieData> new_data) {
  this->data = new_data;
}

std::shared_ptr<Trie> Trie::get_child(unsigned char index) {
  if (index < 0 || index > 9)
    throw TrieInvalidChildIndexException();
  if (!children[index])
    fill_n(children, 10, std::make_shared<Trie>()); // Make the prefix tree grow, someone needs more children
  return children[index];
}

bool Trie::has_child_data(unsigned char index) {
  if (index < 0 || index > 9)
    throw TrieInvalidChildIndexException();
  return children[index]->get_data()->get_rate() != -1;
}

/**
    Inserts rate data in the prefix tree at the correct prefix location
*/
void Trie::insert(std::shared_ptr<Trie> trie, const char *prefix, size_t prefix_length, std::shared_ptr<TrieData> new_data) {
  if (prefix_length < 0)
    throw TrieInvalidInsertionException();
  while (prefix_length > 0) {
    unsigned char child_index = prefix[0] - 48; // convert "0", "1", "2"... to 0, 1, 2,...
    trie = trie->get_child(child_index);
    prefix++;
    prefix_length--;
  }
  std::shared_ptr<TrieData> trie_data = trie->get_data();
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
std::shared_ptr<TrieData> Trie::search(std::shared_ptr<Trie> trie, const char *prefix, size_t prefix_length) {
  if (prefix_length < 0)
    throw TrieInvalidInsertionException();
  std::shared_ptr<TrieData> best_time_data = nullptr;
  while (prefix_length > 0) {
    std::shared_ptr<TrieData> data = trie->get_data();
    if (data && data->get_end_date() != -1 &&   // Only save this date if the rate event has finished
        (!best_time_data || data->get_effective_date() > best_time_data->get_effective_date()))
      best_time_data = data;                    // Save the nearest date to now, since time dimension is important
    unsigned char child_index = prefix[0] - 48; // convert "0", "1", "2"... to 0, 1, 2,...
    if (trie->has_child_data(child_index)) {   // If we have a child node with data, move to it so we can search the longest prefix
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
