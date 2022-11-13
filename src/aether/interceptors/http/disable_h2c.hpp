/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include "aether/proxy/server.hpp"

namespace interceptors {

// Service for disabling h2c upgrade requests.
class disable_h2c : proxy::tcp::intercept::http_interceptor_service::service {
 public:
  void operator()(proxy::connection::connection_flow& flow, proxy::tcp::http::exchange& exch) override;

  inline proxy::tcp::intercept::http_event event() const override {
    return proxy::tcp::intercept::http_event::any_request;
  }
};

}  // namespace interceptors
