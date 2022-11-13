/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "exchange.hpp"

#include "aether/proxy/error/exceptions.hpp"

namespace proxy::http {

http::response& exchange::response() {
  if (!has_response()) {
    throw error::http::no_response_exception{std::string(no_response_error_message)};
  }
  return res_.value();
}

const http::response& exchange::response() const {
  if (!has_response()) {
    throw error::http::no_response_exception{std::string(no_response_error_message)};
  }
  return res_.value();
}

}  // namespace proxy::http
