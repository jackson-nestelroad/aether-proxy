/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "tunnel_loop.hpp"

#include <boost/system/error_code.hpp>
#include <functional>

#include "aether/proxy/connection/base_connection.hpp"
#include "aether/proxy/types.hpp"
#include "aether/util/buffer_segment.hpp"

namespace proxy::tunnel {

tunnel_loop::tunnel_loop(connection::base_connection& source, connection::base_connection& destination)
    : source_(source), destination_(destination) {}

void tunnel_loop::start(callback_t handler) {
  on_finished_ = std::move(handler);
  finished_ = false;
  source_.set_mode(connection::base_connection::io_mode::tunnel);
  // We write first in case something is waiting to be sent before any reads can occur If there is no data to send, the
  // write method returns immediately.
  write();
}

void tunnel_loop::read() { source_.read_async(std::bind_front(&tunnel_loop::on_read, this)); }

void tunnel_loop::on_read(const boost::system::error_code& error, std::size_t) {
  if (error != boost::system::errc::success) {
    finish();
  } else {
    write();
  }
}

void tunnel_loop::write() {
  destination_.output_stream() << source_.input_stream().rdbuf();
  // No timeout because there is a timeout on the read operation already.
  //
  // Timeout cancels ALL socket operations. Attempting to use the same timeout service will cause the latter operation
  // to never timeout.
  destination_.write_untimed_async(std::bind_front(&tunnel_loop::on_write, this));
}

void tunnel_loop::on_write(const boost::system::error_code& error, std::size_t) {
  if (error != boost::system::errc::success) {
    finish();
  } else {
    read();
  }
}

void tunnel_loop::finish() {
  source_.set_mode(connection::base_connection::io_mode::regular);
  finished_ = true;
  on_finished_();
}

}  // namespace proxy::tunnel
