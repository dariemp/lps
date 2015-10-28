#include "code.hxx"
#include <cmath>
#include <cstdint>

using namespace trie;

Code::Code(unsigned long long code) {
  current_code = code;
  zero_counter = 0;
}

bool Code::has_more_digits() {
  return current_code != -1;
}

char Code::next_digit() {
  if (current_code == -1)
    return -1;
  else if (current_code == 0) {
    current_code = -1;
    return 0;
  }
  else if (zero_counter > 0) {
    zero_counter--;
    return 0;
  }
  else {
    unsigned int e = int(floor(log10(current_code)));
    char digit;
    if (e == 0) {
      digit = current_code;
      current_code = -1;
    }
    else {
      long long power10 = pow(10, e);
      digit = uint8_t(floor(current_code / power10));
      long long base10 = digit * power10;
      long long next_code = current_code - base10;
      unsigned int next_e = int(floor(log10(next_code)));
      zero_counter = e - next_e - 1;
      current_code = next_code;
    }
    return digit;
  }
}
