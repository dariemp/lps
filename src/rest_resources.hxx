#ifndef REST_RESOURCES_HXX
#define REST_RESOURCES_HXX

#include <httpserver.hpp>

using namespace httpserver;

namespace rest {

  class RestSearchCode : public http_resource <RestSearchCode> {
  	public:
      void render(const http_request& request, http_response** response);
  };

  class RestSearchCodeName : public http_resource <RestSearchCodeName> {
  	public:
      void render(const http_request& request, http_response** response);
  };

  class RestSearchCodeNameAndRateTable : public http_resource <RestSearchCodeNameAndRateTable> {
  	public:
      void render(const http_request& request, http_response** response);
  };

  class RestSearchRateTable : public http_resource <RestSearchRateTable> {
  	public:
      void render(const http_request& request, http_response** response);
  };

  class RestSearchAllCodeNames : public http_resource <RestSearchAllCodeNames> {
    public:
      void render(const http_request& request, http_response** response);
  };

}

#endif
