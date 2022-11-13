/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <string_view>

#include "aether/proxy/connection/client_connection.hpp"
#include "aether/proxy/connection/server_connection.hpp"
#include "aether/proxy/error/error_state.hpp"
#include "aether/proxy/types.hpp"
#include "aether/util/identifiable.hpp"

namespace proxy::connection {

// A thin wrapper for a connection pair (client and server).
// Will always have a client connection, but server connection can be started later.
// Client and server connections should never be instantiated separately to assure they are kept together.
class connection_flow : public util::id::identifiable<std::size_t> {
 public:
  connection_flow(boost::asio::io_context& ioc, server_components& components);
  connection_flow() = delete;
  ~connection_flow() = default;
  connection_flow(const connection_flow& other) = delete;
  connection_flow& operator=(const connection_flow& other) = delete;
  connection_flow(connection_flow&& other) noexcept = delete;
  connection_flow& operator=(connection_flow&& other) noexcept = delete;

  // Sets the server to connect to later.
  // Any existing server connection is closed.
  void set_server(std::string host, port_t port);

  // Connects to a server.
  // Set server details using set_server.
  void connect_server_async(const err_callback_t& handler);

  // Establishes a TLS connection with the client.
  void establish_tls_with_client_async(tcp::tls::openssl::ssl_server_context_args& args, const err_callback_t& handler);

  // Establishes a TLS connection with the server.
  // Set server details using set_server.
  void establish_tls_with_server_async(tcp::tls::openssl::ssl_context_args& args, const err_callback_t& handler);

  // Disconnects both the client and server connections if applicable.
  void disconnect();

  // Returns if the connection flow should be intercepted using TLS.
  inline bool should_intercept_tls() const { return intercept_tls_; }

  // Marks the connection flow for TLS interception in passthrough mode.
  inline void set_intercept_tls(bool val) { intercept_tls_ = val; }

  // Returns if the connection flow's WebSocket messages should be intercepted.
  inline bool should_intercept_websocket() const { return intercept_websocket_; }

  // Marks the connection flow for WebSocket interception in passthrough mode.
  inline void set_intercept_websocket(bool val) { intercept_websocket_ = val; }

  inline boost::asio::io_context& io_context() const { return ioc_; }

  inline std::string_view target_host() { return target_host_; }
  inline port_t target_port() { return target_port_; }

 private:
  boost::asio::io_context& ioc_;

  // Shared pointers to the two connections to let them use shared_from_this().
  // Private to prevent dangling references.

  std::shared_ptr<base_connection> client_ptr_;
  std::shared_ptr<base_connection> server_ptr_;

  std::string target_host_;
  port_t target_port_;

  bool intercept_tls_;
  bool intercept_websocket_;

 public:
  // "Interfaces" to the shared pointers.
  // Always make sure these are defined after the above pointers so that they are initialized after.

  client_connection& client;
  server_connection& server;

  error::error_state error;
};

}  // namespace proxy::connection
