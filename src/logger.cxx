#include "logger.hxx"

using namespace ctrl;

Logger::Logger() {}

void Logger::log(std::string msg) {
  output_queue.push(msg);
}

void Logger::error(std::string msg) {
  error_queue.push(msg);
}

bool Logger::try_get_output(std::string &output) {
  return output_queue.try_pop(output);
}

bool Logger::try_get_error(std::string &error) {
  return error_queue.try_pop(error);
}

static p_logger_t logger;

p_logger_t Logger::get_logger() {
  if (!logger) {
    logger = new Logger();
  }
  return logger;
}
