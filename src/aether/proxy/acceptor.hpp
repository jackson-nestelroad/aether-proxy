/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <atomic>
#include <boost/asio.hpp>

#include "aether/proxy/connection/connection_flow.hpp"
#include "aether/proxy/server_components.hpp"
#include "aether/proxy/types.hpp"

namespace proxy {

// Wrapping class for boost::asio::ip::tcp::acceptor.
// Accepts new connections.
class acceptor {
 public:
  acceptor(server_components& components);
  acceptor() = delete;
  ~acceptor() = default;
  acceptor(const acceptor& other) = delete;
  acceptor& operator=(const acceptor& other) = delete;
  acceptor(acceptor&& other) noexcept = delete;
  acceptor& operator=(acceptor&& other) noexcept = delete;

  void start();
  void stop();
  void init_accept();
  void on_accept(connection::connection_flow& connection, const boost::system::error_code& error);

  inline boost::asio::ip::tcp::endpoint get_endpoint() const { return endpoint_; }

 private:
  program::options& options_;
  concurrent::io_context_pool& io_contexts_;
  connection::connection_manager& connection_manager_;

  boost::asio::ip::tcp::endpoint endpoint_;
  boost::asio::ip::tcp::acceptor acc_;
  std::atomic<bool> is_stopped_;
};
}  // namespace proxy
