#ifndef CODE_HXX
#define CODE_HXX

namespace trie {

  class Code {
    private:
      long long current_code;
      unsigned char zero_counter;
    public:
      Code(unsigned long long code);
      char next_digit();
      bool has_more_digits();
  };
}

#endif
