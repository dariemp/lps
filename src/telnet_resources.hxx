#ifndef TELNET_RESOURCES_H
#define TELNET_RESOURCES_H

#include <string>
#include <vector>

namespace telnet {

typedef std::vector<std::string> args_t;

class TelnetResource {
  public:
    virtual std::string process_command(std::string input) = 0;
    args_t get_args(std::string input);

};

class TelnetSearchCode : public TelnetResource  {
  public:
    std::string process_command(std::string input);
};

class TelnetSearchCodeName : public TelnetResource  {
  public:
    std::string process_command(std::string input);
};

class TelnetSearchCodeNameAndRateTable : public TelnetResource  {
  public:
    std::string process_command(std::string input);
};

class TelnetSearchRateTable : public TelnetResource  {
  public:
    std::string process_command(std::string input);
};

class TelnetSearchAllCodes : public TelnetResource  {
  public:
    std::string process_command(std::string input);
};
}

#endif
