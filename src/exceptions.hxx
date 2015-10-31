#ifndef EXCEPTIONS_HXX
#define EXCEPTIONS_HXX
#include <exception>

class TrieInvalidPrefixLengthException : public std::exception {
  virtual const char* what() const throw()
    {
      return "Cannot do insertion or search of an empty code in a prefix tree";
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

class TrieMissingChildException : public std::exception {
  virtual const char* what() const throw()
    {
      return "Requested child prefix tree does not exists";
    }
};

class TrieWrongRateTableException : public std::exception {
  virtual const char* what() const throw()
    {
      return "Internal error: traying to insert data in the wrong rate table";
    }
};

class TrieChildAlreadyExistsException : public std::exception {
  virtual const char* what() const throw()
    {
      return "Child prefix tree already exists, please edit instead of insert";
    }
};

class TrieCollisionSameRateException : public std::exception {
  virtual const char* what() const throw()
    {
      return "Collition detected while inserting new data in the prefix tree: same prefix, same rate, different dates";
    }
};

class TrieCollisionDifferentRateException : public std::exception {
  virtual const char* what() const throw()
    {
      return "Collition detected while inserting new data in the prefix tree: same prefix, different rate";
    }
};

class WorkerReleaseLeakException : public std::exception {
  virtual const char* what() const throw()
    {
      return "Internal Error: Trying to release memory in the wrong worker thread";
    }
};

class DBNoConnectionsException : public std::exception {
  virtual const char* what() const throw()
    {
      return "At least one connection to the database is needed";
    }
};

class DBQueryFailedException : public std::exception {
  virtual const char* what() const throw()
    {
      return "Database queries are failing";
    }
};

class RestRequestArgException : public std::exception {
  virtual const char* what() const throw()
    {
      return "Invalid parameter value";
    }
};

class ControllerNoInstanceException : public std::exception {
  virtual const char* what() const throw()
    {
      return "Internal error: no controller object";
    }
};
#endif
