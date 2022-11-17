/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "timeout_service.hpp"

#include <boost/asio.hpp>
#include <functional>

#include "aether/proxy/types.hpp"

namespace proxy::connection {
timeout_service::timeout_service(boost::asio::io_context& ioc) : timer_(ioc) {}

void timeout_service::reset_timer() { timer_.expires_from_now(boost::posix_time::pos_infin); }

void timeout_service::on_timeout(callback_t handler, const boost::system::error_code& error) {
  // Timer was not canceled.
  if (error != boost::asio::error::operation_aborted) {
    reset_timer();
    handler();
  }
}

void timeout_service::set_timeout(const milliseconds& time, callback_t handler) {
  timer_.expires_from_now(time);
  timer_.async_wait(std::bind_front(&timeout_service::on_timeout, this, std::move(handler)));
}

void timeout_service::cancel_timeout() {
  timer_.cancel();
  reset_timer();
}

}  // namespace proxy::connection
