/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include "aether/proxy/server.hpp"

namespace interceptors {

// Service for disabling h2c upgrade requests.
class disable_h2c : proxy::intercept::http_interceptor_service::service {
 public:
  void operator()(proxy::connection::connection_flow& flow, proxy::http::exchange& exch) override;

  inline proxy::intercept::http_event event() const override { return proxy::intercept::http_event::any_request; }
};

}  // namespace interceptors
