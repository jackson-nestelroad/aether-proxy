/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/system/error_code.hpp>

#include "aether/proxy/connection/base_connection.hpp"
#include "aether/proxy/types.hpp"
#include "aether/util/buffer_segment.hpp"

namespace proxy::tunnel {

// Implements an asynchronous read/write loop from one connection to another.
// Connections must outlive any tunnel loop it is connected to.
class tunnel_loop {
 public:
  tunnel_loop(connection::base_connection& source, connection::base_connection& destination);

  void start(callback_t handler);
  inline bool finished() const { return finished_; }

 private:
  void read();
  void on_read(const boost::system::error_code& error, std::size_t bytes_transferred);
  void write();
  void on_write(const boost::system::error_code& error, std::size_t bytes_transferred);
  void finish();

  connection::base_connection& source_;
  connection::base_connection& destination_;
  callback_t on_finished_;
  bool finished_;
};
}  // namespace proxy::tunnel
