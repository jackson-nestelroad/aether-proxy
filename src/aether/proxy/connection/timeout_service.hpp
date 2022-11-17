/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>

#include "aether/proxy/types.hpp"

namespace proxy::connection {

// Service to timeout connect, read, and write requests on the connection class.
class timeout_service {
 public:
  timeout_service(boost::asio::io_context& ioc);
  timeout_service() = delete;
  ~timeout_service() = default;
  timeout_service(const timeout_service& other) = delete;
  timeout_service& operator=(const timeout_service& other) = delete;
  timeout_service(timeout_service&& other) noexcept = delete;
  timeout_service& operator=(timeout_service&& other) noexcept = delete;

  // Set the timer to call the handler in a given amount of time.
  void set_timeout(const milliseconds& time, callback_t handler);

  // Cancel the timeout, preventing the previous set_timeout handler to be called.
  void cancel_timeout();

 private:
  // Resets the timer to infinity.
  void reset_timer();

  // Callback for the timer function.
  // Will call if the timer is met or if the timer is cancelled.
  void on_timeout(callback_t handler, const boost::system::error_code& error);

  boost::asio::deadline_timer timer_;
};

}  // namespace proxy::connection
