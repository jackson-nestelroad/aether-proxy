/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "disable_h2c.hpp"

namespace interceptors {

void disable_h2c::operator()(proxy::connection::connection_flow& flow, proxy::tcp::http::exchange& exch) {
  auto& req = exch.request();
  if (req.header_has_value("Upgrade", "h2c")) {
    req.remove_header("Upgrade");
  }
}

}  // namespace interceptors
