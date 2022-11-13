/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <map>
#include <mutex>
#include <set>

#include "aether/proxy/connection/connection_flow.hpp"
#include "aether/proxy/connection_handler.hpp"
#include "aether/proxy/tcp/intercept/interceptor_services.hpp"

namespace proxy::connection {

// Small class to manage ongoing connection flows.
// Owns connection flows and connection handlers to assure they are not destroyed until their work is finished.
class connection_manager {
 public:
  connection_manager(server_components& components);
  ~connection_manager() = default;
  connection_manager(const connection_manager& other) = delete;
  connection_manager& operator=(const connection_manager& other) = delete;
  connection_manager(connection_manager&& other) noexcept = delete;
  connection_manager& operator=(connection_manager&& other) noexcept = delete;

  connection_flow& new_connection(boost::asio::io_context& ioc);

  // Starts managing and handling a new connection flow.
  // This should be called after a client has connected.
  void start(connection_flow& flow);

  // Destroys a given connection, without regards to the service it is connected to.
  // This method should only be called if the connection has not been given to a connection_handler instance.
  void destroy(connection_flow& flow);

  // Stop all connections immediately.
  void stop_all();

  // Returns the total number of connections to the proxy.
  inline std::size_t total_connection_count() const { return connections_.size(); }

  // Returns the total number of connections being serviced.
  inline std::size_t active_connection_count() const { return services_.size(); }

  // Returns the total number of connections awaiting service.
  inline std::size_t pending_connection_count() const { return 0; }

 private:
  // Starts servicing a connection.
  void start_service(connection_flow& flow);

  // Stops an existing service, deleting it from the records.
  void stop(const std::unique_ptr<connection_handler>& service_ptr);

  std::mutex data_mutex_;
  std::map<connection_flow::id_t, std::unique_ptr<connection_flow>> connections_;
  std::set<std::unique_ptr<connection_handler>> services_;

  server_components& components_;
};

}  // namespace proxy::connection
