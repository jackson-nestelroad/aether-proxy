/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/system/error_code.hpp>

#include "aether/proxy/connection/connection_flow.hpp"
#include "aether/proxy/tcp/base_service.hpp"
#include "aether/proxy/tcp/tunnel/tunnel_loop.hpp"

namespace proxy::tcp::tunnel {
// Service for a TCP tunnel between two sockets.
// No interception is possible here.
class tunnel_service : public base_service {
 public:
  tunnel_service(connection::connection_flow& flow, connection_handler& owner, server_components& components);

  void start() override;

 private:
  void connect_server();
  void on_connect_server(const boost::system::error_code& error);
  void initiate_tunnel();
  void on_finish();

  tunnel_loop upstream_;
  tunnel_loop downstream_;
};

}  // namespace proxy::tcp::tunnel
