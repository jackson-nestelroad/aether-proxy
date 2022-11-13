/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>
#include <functional>

namespace util {

// Class for handling exit signals.
// Runs its own io_context and thread, separate from the server.
class signal_handler {
 public:
  using callback_t = std::function<void()>;

  signal_handler(boost::asio::io_context& ioc);
  ~signal_handler();
  void wait(const callback_t& handler);
  void pause();
  void unpause();

 private:
  void on_signal(const boost::system::error_code& error, int signo, const callback_t& handler);

  boost::asio::signal_set signals_;
  bool paused_;
};

}  // namespace util
