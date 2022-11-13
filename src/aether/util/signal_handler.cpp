/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "signal_handler.hpp"

#include <boost/bind/bind.hpp>
#include <csignal>

#include "aether/util/console.hpp"

namespace util {

signal_handler::signal_handler(boost::asio::io_context& ioc) : signals_(ioc), paused_(false) {
  signals_.add(SIGINT);
  signals_.add(SIGTERM);
#if defined(SIGQUIT)
  signals_.add(SIGQUIT);
#endif
}

signal_handler::~signal_handler() { signals_.cancel(); }

void signal_handler::wait(const callback_t& callback) {
  signals_.async_wait(boost::bind(&signal_handler::on_signal, this, boost::asio::placeholders::error,
                                  boost::asio::placeholders::signal_number, callback));
}

void signal_handler::pause() { paused_ = true; }

void signal_handler::unpause() { paused_ = false; }

void signal_handler::on_signal(const boost::system::error_code& error, int signo, const callback_t& handler) {
  if (error == boost::system::errc::success) {
    if (paused_) {
      wait(handler);
    } else {
      handler();
    }
  }
}

}  // namespace util
