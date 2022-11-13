/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "base_service.hpp"

#include <algorithm>
#include <boost/bind/bind.hpp>
#include <string_view>

#include "aether/proxy/connection/connection_flow.hpp"
#include "aether/proxy/connection_handler.hpp"
#include "aether/proxy/intercept/interceptor_services.hpp"
#include "aether/proxy/server_components.hpp"

namespace proxy {
base_service::base_service(connection::connection_flow& flow, connection_handler& owner, server_components& components)
    : ioc_(flow.io_context()),
      options_(components.options),
      flow_(flow),
      owner_(owner),
      interceptors_(components.interceptors) {}

base_service::~base_service() {}

void base_service::set_server(std::string host, port_t port) {
  if (!flow_.server.is_connected_to(host, port)) {
    flow_.set_server(std::move(host), port);
    if (flow_.server.connected()) {
      interceptors_.server.run(intercept::server_event::disconnect, flow_);
    }
    bool is_forbidden =
        std::find(forbidden_hosts.begin(), forbidden_hosts.end(), flow_.target_host()) != forbidden_hosts.end() &&
        flow_.target_port() == options_.port;
    if (is_forbidden) {
      throw error::self_connect_exception{};
    }
  }
}

void base_service::connect_server_async(const err_callback_t& handler) {
  flow_.connect_server_async(
      boost::bind(&base_service::on_connect_server, this, boost::asio::placeholders::error, handler));
}

void base_service::on_connect_server(const boost::system::error_code& error, const err_callback_t& handler) {
  if (error == boost::system::errc::success) {
    interceptors_.server.run(intercept::server_event::connect, flow_);
  }
  handler(error);
}

void base_service::stop() { owner_.stop(); }

}  // namespace proxy
