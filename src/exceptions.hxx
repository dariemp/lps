#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H
#include <exception>

class TrieInvalidInsertionException : public std::exception {
  virtual const char* what() const throw()
    {
      return "Cannot do insertion of and empty prefix in a prefix tree";
    }
};

class TrieInvalidPrefixDigitException : public std::exception {
  virtual const char* what() const throw()
    {
      return "Prefix digit out of range: must be a number between 0 and 9";
    }
};

class TrieInvalidChildIndexException : public std::exception {
  virtual const char* what() const throw()
    {
      return "Prefix tree child index out of range: must be a number between 0 and 9";
    }
};

class TrieCollisionException : public std::exception {
  virtual const char* what() const throw()
    {
      return "Collition detected while inserting new data in the prefix tree";
    }
};
#endif
