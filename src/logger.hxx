#ifndef LOGGER_HXX
#define LOGGER_HXX

#include <tbb/tbb.h>

namespace ctrl {
  class Logger;
  typedef Logger* p_logger_t;

  class Logger {
    private:
      typedef tbb::concurrent_queue<std::string> output_queue_t;
      output_queue_t output_queue;
      output_queue_t error_queue;
      Logger();
    public:
      static p_logger_t get_logger();
      void log(std::string msg);
      void error(std::string msg);
      bool try_get_output(std::string &output);
      bool try_get_error(std::string &error);
  };

  using std::placeholders::_1;
  static std::function<void(std::string)> log = std::bind(&Logger::log, Logger::get_logger(), _1);
  static std::function<void(std::string)> error = std::bind(&Logger::error, Logger::get_logger(), _1);
  static std::function<bool(std::string&)> try_get_output = std::bind(&Logger::try_get_output, Logger::get_logger(), _1);
  static std::function<bool(std::string&)> try_get_error = std::bind(&Logger::try_get_error, Logger::get_logger(), _1);
}

#endif
