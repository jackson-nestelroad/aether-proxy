/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "connection_handler.hpp"

#include "aether/proxy/connection/connection_flow.hpp"
#include "aether/proxy/server_components.hpp"
#include "aether/proxy/tcp/http/http1/http_service.hpp"

namespace proxy {
connection_handler::connection_handler(connection::connection_flow& flow, server_components& components)
    : ioc_(flow.io_context()), flow_(flow), components_(components) {}

void connection_handler::start(const callback_t& handler) {
  on_finished_ = handler;
  current_service_ = std::make_unique<tcp::http::http1::http_service>(flow_, *this, components_);
  current_service_->start();
}

void connection_handler::stop() {
  flow_.disconnect();
  current_service_.reset();
  on_finished_();
}

}  // namespace proxy
