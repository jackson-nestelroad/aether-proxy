/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "attach.hpp"

#include "aether/interceptors/http/disable_h2c.hpp"
#include "aether/interceptors/http/http_logger.hpp"

namespace interceptors {

void attach_default(proxy::server& server) {
  out::debug::log("Attaching default interceptors");
  server.interceptors().http.attach<disable_h2c>();
}

}  // namespace interceptors
