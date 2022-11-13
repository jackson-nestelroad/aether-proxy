/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "tunnel_service.hpp"

#include <boost/bind/bind.hpp>

#include "aether/proxy/connection/connection_flow.hpp"
#include "aether/proxy/connection_handler.hpp"
#include "aether/proxy/server_components.hpp"

namespace proxy::tcp::tunnel {
tunnel_service::tunnel_service(connection::connection_flow& flow, connection_handler& owner,
                               server_components& components)
    : base_service(flow, owner, components),
      upstream_(static_cast<connection::base_connection&>(flow.client),
                static_cast<connection::base_connection&>(flow.server)),
      downstream_(static_cast<connection::base_connection&>(flow.server),
                  static_cast<connection::base_connection&>(flow.client)) {}

void tunnel_service::start() {
  if (!flow_.server.connected()) {
    connect_server();
  } else {
    initiate_tunnel();
  }
}

void tunnel_service::connect_server() {
  connect_server_async(boost::bind(&tunnel_service::on_connect_server, this, boost::asio::placeholders::error));
}

void tunnel_service::on_connect_server(const boost::system::error_code& error) {
  if (error != boost::system::errc::success) {
    flow_.error.set_boost_error(error);
    stop();
  } else {
    initiate_tunnel();
  }
}

void tunnel_service::initiate_tunnel() {
  interceptors_.tunnel.run(intercept::tunnel_event::start, flow_);
  downstream_.start(boost::bind(&tunnel_service::on_finish, this));
  upstream_.start(boost::bind(&tunnel_service::on_finish, this));
}

void tunnel_service::on_finish() {
  if (downstream_.finished() && upstream_.finished()) {
    interceptors_.tunnel.run(intercept::tunnel_event::stop, flow_);
    stop();
  }
}
}  // namespace proxy::tcp::tunnel
