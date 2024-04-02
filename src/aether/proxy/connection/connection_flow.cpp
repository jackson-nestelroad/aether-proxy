/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "connection_flow.hpp"

#include <boost/asio.hpp>
#include <memory>
#include <string_view>
#include <utility>

#include "aether/proxy/connection/client_connection.hpp"
#include "aether/proxy/connection/server_connection.hpp"
#include "aether/proxy/error/error.hpp"
#include "aether/proxy/error/error_state.hpp"
#include "aether/proxy/server_components.hpp"
#include "aether/proxy/types.hpp"

namespace proxy::connection {
connection_flow::connection_flow(boost::asio::io_context& ioc, server_components& components)
    : ioc_(ioc),
      id_(components.uuid_factory.v1()),
      target_port_(),
      intercept_tls_(false),
      intercept_websocket_(false),
      client(ioc_, components),
      server(ioc_, components) {
  out::safe_debug::log("New connection flow", id());
}

connection_flow::~connection_flow() { out::safe_debug::log("Deleting connection flow", id()); }

void connection_flow::set_server(std::string host, port_t port) {
  if (server.connected()) {
    server.disconnect();
  }
  target_host_ = std::move(host);
  target_port_ = port;
}

void connection_flow::connect_server_async(err_callback_t handler) {
  server.connect_async(target_host_, target_port_, std::move(handler));
}

result<void> connection_flow::establish_tls_with_client_async(tls::openssl::ssl_server_context_args& args,
                                                              err_callback_t handler) {
  return client.establish_tls_async(args, std::move(handler));
}

result<void> connection_flow::establish_tls_with_server_async(tls::openssl::ssl_context_args& args,
                                                              err_callback_t handler) {
  return server.establish_tls_async(args, std::move(handler));
}

void connection_flow::disconnect() {
  client.disconnect();
  server.disconnect();
}

}  // namespace proxy::connection
