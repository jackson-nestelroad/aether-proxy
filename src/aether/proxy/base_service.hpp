/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <array>
#include <boost/asio.hpp>
#include <string>
#include <string_view>

#include "aether/proxy/connection/connection_flow.hpp"
#include "aether/proxy/intercept/interceptor_services.hpp"
#include "aether/proxy/types.hpp"

namespace proxy {
class server_components;
class connection_handler;
}  // namespace proxy

namespace proxy {

// Provides fundamental methods and data for all specialized services.
// All specialized services must inherit this class.
class base_service {
 public:
  base_service(connection::connection_flow& flow, connection_handler& owner, server_components& components);
  base_service() = delete;
  virtual ~base_service();
  base_service(const base_service& other) = delete;
  base_service& operator=(const base_service& other) = delete;
  base_service(base_service&& other) noexcept = delete;
  base_service& operator=(base_service&& other) noexcept = delete;

  // Starts the service asynchronously.
  // Once the service finishes, the finished callback passed to this method will run.
  virtual void start() = 0;

  // Stops the service or signals the end of the service.
  void stop();

 protected:
  // Sets the server to connect to later.
  void set_server(std::string host, port_t port);

  // Connects to the server asynchronously.
  void connect_server_async(const err_callback_t& handler);

  boost::asio::io_context& ioc_;
  program::options& options_;
  connection::connection_flow& flow_;
  connection_handler& owner_;
  intercept::interceptor_manager& interceptors_;

 private:
  static constexpr std::array<std::string_view, 3> forbidden_hosts = {"localhost", "127.0.0.1", "::1"};

  void on_connect_server(const boost::system::error_code& error, const err_callback_t& handler);
};
}  // namespace proxy
