/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "exchange.hpp"

#include "aether/proxy/error/error.hpp"

namespace proxy::http {

http::response& exchange::response() {
  if (!has_response()) {
    make_response();
  }
  return res_.value();
}

const http::response& exchange::response() const {
  if (!has_response()) {
    make_response();
  }
  return res_.value();
}

}  // namespace proxy::http
