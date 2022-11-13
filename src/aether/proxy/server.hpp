/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <thread>

#include "aether/proxy/acceptor.hpp"
#include "aether/proxy/server_components.hpp"
#include "aether/proxy/types.hpp"
#include "aether/util/signal_handler.hpp"
#include "aether/util/thread_blocker.hpp"

namespace proxy {
// The server class used to startup all of the boost::asio:: services.
// Manages the acceptor port and io_context pool.
class server {
 public:
  server(const program::options& options);
  ~server();
  server(const server& other) = delete;
  server& operator=(const server& other) = delete;
  server(server&& other) noexcept = delete;
  server& operator=(server&& other) noexcept = delete;

  void start();

  // Stops the server and calls cleanup methods.
  void stop();
  void pause_signals();
  void unpause_signals();
  void enable_logs();
  void disable_logs();

  // Blocks the thread until the server is stopped internally.
  // The server can stop using the stop() function or using exit signals registered on the signal_handler.
  void await_stop();

  bool running() const;
  std::string endpoint_string() const;
  boost::asio::io_context& get_io_context();

  // Expose interceptors so methods and hubs can be attached from the outside world.
  inline tcp::intercept::interceptor_manager& interceptors() { return components_.interceptors; }

 private:
  server_components components_;

  bool is_running_;
  bool needs_cleanup_;

  out::logging_manager log_manager_;
  std::unique_ptr<acceptor> acc_;
  std::unique_ptr<util::signal_handler> signals_;
  util::thread_blocker blocker_;

  // Method for calling io_context.run() to start the boost::asio:: services.
  static void run_io_context(boost::asio::io_context& ioc);

  void signal_stop();

  // Cleans up the server, if necessary.
  void cleanup();
};
}  // namespace proxy
