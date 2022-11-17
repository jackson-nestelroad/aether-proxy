/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <type_traits>

#include "aether/proxy/base_service.hpp"
#include "aether/proxy/connection/connection_flow.hpp"

namespace proxy {

// Handles a connection by passing it to a service.
class connection_handler : public std::enable_shared_from_this<connection_handler> {
 public:
  connection_handler(connection::connection_flow& flow, server_components& components);
  connection_handler() = delete;
  ~connection_handler() = default;
  connection_handler(const connection_handler& other) = delete;
  connection_handler& operator=(const connection_handler& other) = delete;
  connection_handler(connection_handler&& other) noexcept = delete;
  connection_handler& operator=(connection_handler&& other) noexcept = delete;

  // Starts handling the connection by routing it to specialized services.
  // Once the connection finishes and is disconnected, the callback passed here will be called.
  void start(callback_t handler);

  // Immediately switches the flow to a new service to be handled.
  template <typename T, typename... Args>
  std::enable_if_t<std::is_base_of_v<base_service, T>, void> switch_service(Args&... args) {
    current_service_ = std::make_unique<T>(flow_, *this, components_, args...);
    current_service_->start();
  }

  // Stops handling the connection by disconnecting from the client and server.
  // Calls the finished callback passed when the handler was started.
  void stop();

  inline connection::connection_flow& connection_flow() const { return flow_; }

 private:
  boost::asio::io_context& ioc_;
  callback_t on_finished_;

  // The single connection flow being managed
  connection::connection_flow& flow_;

  // The current service handling the connection flow
  std::unique_ptr<base_service> current_service_;

  server_components& components_;
};

}  // namespace proxy
