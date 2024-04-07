/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "server.hpp"

#include <boost/asio.hpp>
#include <exception>
#include <sstream>
#include <string>

#include "aether/program/options.hpp"
#include "aether/proxy/error/error.hpp"
#include "aether/util/console.hpp"
#include "aether/util/result.hpp"
#include "aether/util/result_macros.hpp"
#include "aether/util/signal_handler.hpp"

namespace proxy {

server::server(program::options options)
    : components_(std::move(options)), is_running_(false), needs_cleanup_(false), log_manager_() {
  // All logs are silenced in interactive mode until manually unsilenced.
  if (components_.options.run_interactive) {
    log_manager_.silence();
  }

  // Logs may be redirected to a log file, even in command service mode.
  if (!components_.options.log_file_name.empty()) {
    log_manager_.redirect_to_file(components_.options.log_file_name);
  }

  // The silent option overrides any log file setting.
  if (components_.options.run_silent) {
    log_manager_.silence();
  }
}

server::~server() { stop(); }

void run_io_context(boost::asio::io_context& ioc) {
  while (true) {
    try {
      ioc.run();
      break;
    } catch (const std::exception& ex) {
      out::safe_error::log("Unexpected exception running io_context instance:", ex.what());
    }
  }
}

result<void> server::start() {
  out::debug::log("Starting server");

  is_running_ = true;
  needs_cleanup_ = true;
  signals_ = std::make_unique<util::signal_handler>(components_.io_contexts.get_io_context());

  signals_->wait(std::bind_front(&server::signal_stop, this));

  ASSIGN_OR_RETURN(acc_, acceptor::create(components_));
  acc_->start();
  components_.io_contexts.run(run_io_context);

  return util::ok;
}

// When THE server is stopped with signals, it stops running but is not cleaned up here.
//
// Since the signal handler is running on the same threads as the server, we cannot clean up here without a resource
// deadlock. Thus, the needs_cleanup_ flag signals another thread to clean things up
void server::signal_stop() {
  is_running_ = false;
  blocker_.unblock();
}

void server::stop() {
  signal_stop();
  cleanup();
}

void server::cleanup() {
  if (needs_cleanup_) {
    if (acc_) {
      acc_->stop();
    }
    components_.io_contexts.stop();
    signals_.reset();
    blocker_.unblock();
    needs_cleanup_ = false;
  }
}

void server::await_stop() {
  if (is_running_) {
    blocker_.block();
  }
  cleanup();
}

result<void> server::pause_signals() {
  if (!signals_) {
    return error::invalid_operation("Cannot pause signals when server is not running.");
  }
  signals_->pause();
  return util::ok;
}

result<void> server::unpause_signals() {
  if (!signals_) {
    return error::invalid_operation("Cannot unpause signals when server is not running.");
  }
  signals_->unpause();
  return util::ok;
}

void server::enable_logs() { log_manager_.unsilence(); }

void server::disable_logs() { log_manager_.silence(); }

bool server::running() const { return is_running_; }

result<std::string> server::endpoint_string() const {
  if (!acc_) {
    return error::invalid_operation("Cannot access port before server has started. Call server.start() first.");
  }
  std::stringstream str;
  str << acc_->get_endpoint();
  return str.str();
}

boost::asio::io_context& server::get_io_context() { return components_.io_contexts.get_io_context(); }

size_t server::num_connections() const { return components_.connection_manager.total_connection_count(); }

size_t server::num_ssl_certificates() const { return components_.server_store().num_certificates(); }

}  // namespace proxy
