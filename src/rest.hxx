#include <httpserver.hpp>

using namespace httpserver;

namespace rest {

  class Rest : public http_resource<Rest> {
  	public:
      void run_server(unsigned int http_listen_port);
  };
}
