/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>

#include "aether/util/any_invocable.hpp"

namespace util {

// Class for handling exit signals.
// Runs its own io_context and thread, separate from the server.
class signal_handler {
 public:
  using callback_t = util::any_invocable<void()>;

  signal_handler(boost::asio::io_context& ioc);
  ~signal_handler();
  void wait(callback_t handler);
  void pause();
  void unpause();

 private:
  void on_signal(const boost::system::error_code& error, int signo, callback_t handler);

  boost::asio::signal_set signals_;
  bool paused_;
};

}  // namespace util
