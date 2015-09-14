#include "trie.hxx"
using namespace trie;

RateTableData::RateTableData(unsigned int rate_table_id, double rate)
  :rate_table_id(rate_table_id), rate(rate), next(nullptr) {}

std::shared_ptr<RateTableData> RateTableData::get_next() {
  return next;
}

void RateTableData::set_next(std::shared_ptr<RateTableData> next) {
  next = next;
}

unsigned int RateTableData::get_rate_table_id() {
  return rate_table_id;
}

double RateTableData::get_rate() {
  return rate;
}

Trie::Trie():data(nullptr) {
  fill_n(children, 10, nullptr);
}

/**
    Inserts rate data in the current node of the prefix tree
*/
void Trie::insert_data(unsigned int rate_table_id, double rate) {
  if (!data)
    data = std::make_shared<RateTableData>(rate_table_id, rate);
  else if (rate > data->get_rate()) {
    std::shared_ptr<RateTableData> new_node = std::make_shared<RateTableData>(rate_table_id, rate);
    new_node->set_next(data);
    data = new_node;
  }
  else {
    std::shared_ptr<RateTableData> cur_ptr = data;
    std::shared_ptr<RateTableData> next_ptr = data->get_next();
    while (next_ptr && rate < next_ptr->get_rate()) {
      cur_ptr = next_ptr;
      next_ptr = next_ptr->get_next();
    }
    if (next_ptr) {
      while (next_ptr && rate == next_ptr->get_rate() && next_ptr->get_rate_table_id() != rate_table_id) {
        cur_ptr = next_ptr;
        next_ptr = next_ptr->get_next();
      }
      if (next_ptr && (rate > next_ptr->get_rate() || next_ptr->get_rate_table_id() != rate_table_id)) {
        std::shared_ptr<RateTableData> new_node = std::make_shared<RateTableData>(rate_table_id, rate);
        new_node->set_next(next_ptr);
        cur_ptr->set_next(new_node);
      }
    }
    if (!next_ptr)
      cur_ptr->set_next(std::make_shared<RateTableData>(rate_table_id, rate));
  }
};

/**
    Inserts rate data in the prefix tree at the correct prefix location
*/
void Trie::insert(unsigned char *prefix, size_t prefix_length, unsigned int rate_table_id, double rate) {
  if (prefix_length < 1)
    throw TrieInvalidInsertionException();
  unsigned char prefix_first_digit = prefix[0] - 48; // convert "0", "1", "2"... to 0, 1, 2,...
  if (prefix_first_digit < 0 || prefix_first_digit > 9)
    throw TrieInvalidPrefixDigitException();
  if (!children[0])     // Make the prefix tree grow for possible future insertions
    fill_n(children, 10, std::make_shared<Trie>());
  if (prefix_length == 1)
    insert_data(rate_table_id, rate);
  else {
    prefix++;
    prefix_length--;
    unsigned char child_index = prefix[0] - 48; // convert "0", "1", "2"... to 0, 1, 2,...
    if (child_index < 0 || child_index > 9)
      throw TrieInvalidPrefixDigitException();
    std::shared_ptr<Trie> child = children[child_index];
    child->insert(prefix, prefix_length, rate_table_id, rate);
  }
}
