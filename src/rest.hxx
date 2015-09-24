#include <httpserver.hpp>
#include <iostream>

using namespace httpserver;

namespace rest {

  class Rest : public http_resource<Rest> {
  	public:
      void render(const http_request&, http_response**);
      void run_server(unsigned int http_listen_port);
  };
}
