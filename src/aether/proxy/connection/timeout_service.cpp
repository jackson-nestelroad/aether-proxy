/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "timeout_service.hpp"

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

#include "aether/proxy/types.hpp"

namespace proxy::connection {
timeout_service::timeout_service(boost::asio::io_context& ioc) : ioc_(ioc), timer_(ioc) {}

void timeout_service::reset_timer() { timer_.expires_from_now(boost::posix_time::pos_infin); }

void timeout_service::on_timeout(const callback_t& handler, const boost::system::error_code& error) {
  // Timer was not canceled.
  if (error != boost::asio::error::operation_aborted) {
    reset_timer();
    handler();
  }
}

void timeout_service::set_timeout(const milliseconds& time, const callback_t& handler) {
  timer_.expires_from_now(time);
  timer_.async_wait(boost::bind(&timeout_service::on_timeout, this, handler, boost::asio::placeholders::error));
}

void timeout_service::cancel_timeout() {
  timer_.cancel();
  reset_timer();
}

}  // namespace proxy::connection
