/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "base_connection.hpp"

#include <algorithm>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>

#include "aether/proxy/connection/timeout_service.hpp"
#include "aether/proxy/server_components.hpp"
#include "aether/proxy/tls/openssl/ssl_context.hpp"
#include "aether/proxy/types.hpp"
#include "aether/util/console.hpp"

namespace proxy::connection {

base_connection::base_connection(boost::asio::io_context& ioc, server_components& components)
    : options_(components.options),
      ioc_(ioc),
      // TODO: boost::asio::detail::win_mutex leak.
      strand_(boost::asio::make_strand(ioc)),
      socket_(strand_),
      timeout_(ioc),
      mode_(io_mode::regular),
      connected_(false),
      tls_established_(false),
      ssl_context_(),
      cert_(nullptr),
      secure_socket_(),
      read_state_(operation_state::free),
      write_state_(operation_state::free) {}

base_connection::~base_connection() {
  if (socket_.is_open()) {
    close();
  }
}

void base_connection::set_timeout() {
  switch (mode_) {
    case io_mode::regular:
      timeout_.set_timeout(options_.timeout, std::bind_front(&base_connection::on_timeout, this));
      return;
    case io_mode::tunnel:
      timeout_.set_timeout(options_.tunnel_timeout, std::bind_front(&base_connection::on_timeout, this));
      return;
    case io_mode::no_timeout:
      return;
  }
}

bool base_connection::has_been_closed() {
  boost::system::error_code error;

  socket_.non_blocking(true);
  socket_.receive(input_.prepare_sequence(1), boost::asio::ip::tcp::socket::message_peek, error);
  socket_.non_blocking(false);

  if (error == boost::asio::error::eof) {
    return true;
  }
  return false;
}

std::size_t base_connection::read(boost::system::error_code& error) { return read(default_buffer_size, error); }

std::size_t base_connection::read(std::size_t buffer_size, boost::system::error_code& error) {
  set_reading();
  set_timeout();

  std::size_t bytes_read = 0;
  if (tls_established_) {
    bytes_read = secure_socket_->read_some(input_.prepare_sequence(buffer_size), error);
  } else {
    bytes_read = socket_.read_some(input_.prepare_sequence(buffer_size), error);
  }

  input_.commit(bytes_read);
  timeout_.cancel_timeout();
  finish_reading();
  return bytes_read;
}

std::size_t base_connection::read_available(boost::system::error_code& error) {
  set_reading();
  socket_.non_blocking(true);

  std::size_t bytes_read = 0;
  if (tls_established_) {
    bytes_read = boost::asio::read(socket_, util::buffer::asio_dynamic_buffer_v1(input_), error);
  } else {
    bytes_read = boost::asio::read(*secure_socket_, util::buffer::asio_dynamic_buffer_v1(input_), error);
  }

  if (error == boost::asio::error::would_block) {
    error = boost::system::errc::make_error_code(boost::system::errc::success);
  }
  socket_.non_blocking(false);
  finish_reading();
  return bytes_read;
}

void base_connection::read_async(io_callback_t handler) { read_async(default_buffer_size, std::move(handler)); }

void base_connection::read_async(std::size_t buffer_size, io_callback_t handler) {
  set_reading();
  set_timeout();
  if (tls_established_) {
    secure_socket_->async_read_some(input_.prepare_sequence(buffer_size),
                                    boost::asio::bind_executor(strand_, [this, handler = std::move(handler)](
                                                                            const boost::system::error_code& error,
                                                                            std::size_t bytes_transferred) mutable {
                                      on_read_need_to_commit(std::move(handler), error, bytes_transferred);
                                    }));
  } else {
    socket_.async_read_some(input_.prepare_sequence(buffer_size),
                            boost::asio::bind_executor(
                                strand_, [this, handler = std::move(handler)](const boost::system::error_code& error,
                                                                              std::size_t bytes_transferred) mutable {
                                  on_read_need_to_commit(std::move(handler), error, bytes_transferred);
                                }));
  }
}

void base_connection::read_until_async(std::string_view delim, io_callback_t handler) {
  set_reading();
  set_timeout();
  if (tls_established_) {
    boost::asio::async_read_until(*secure_socket_, util::buffer::asio_dynamic_buffer_v1(input_), delim,
                                  boost::asio::bind_executor(strand_, [this, handler = std::move(handler)](
                                                                          const boost::system::error_code& error,
                                                                          std::size_t bytes_transferred) mutable {
                                    on_read(std::move(handler), error, bytes_transferred);
                                  }));
  } else {
    boost::asio::async_read_until(socket_, util::buffer::asio_dynamic_buffer_v1(input_), delim,
                                  boost::asio::bind_executor(strand_, [this, handler = std::move(handler)](
                                                                          const boost::system::error_code& error,
                                                                          std::size_t bytes_transferred) mutable {
                                    on_read(std::move(handler), error, bytes_transferred);
                                  }));
  }
}

void base_connection::on_read(io_callback_t handler, const boost::system::error_code& error,
                              std::size_t bytes_transferred) {
  finish_reading();
  timeout_.cancel_timeout();
  if (error != boost::system::errc::success) {
    set_connected(false);
  }
  boost::asio::post(
      ioc_, [handler = std::move(handler), error, bytes_transferred]() mutable { handler(error, bytes_transferred); });
}

void base_connection::on_read_need_to_commit(io_callback_t handler, const boost::system::error_code& error,
                                             std::size_t bytes_transferred) {
  input_.commit(bytes_transferred);
  on_read(std::move(handler), error, bytes_transferred);
}

std::size_t base_connection::write(boost::system::error_code& error) {
  set_writing();
  set_timeout();

  std::size_t bytes_written = 0;
  if (tls_established_) {
    bytes_written = boost::asio::write(*secure_socket_, util::buffer::asio_dynamic_buffer_v1(output_), error);
  } else {
    bytes_written = boost::asio::write(socket_, util::buffer::asio_dynamic_buffer_v1(output_), error);
  }

  timeout_.cancel_timeout();
  finish_writing();

  return bytes_written;
}

void base_connection::write_async(io_callback_t handler) {
  set_writing();
  set_timeout();
  if (tls_established_) {
    boost::asio::async_write(*secure_socket_, util::buffer::asio_dynamic_buffer_v1(output_),
                             boost::asio::bind_executor(
                                 strand_, [this, handler = std::move(handler)](const boost::system::error_code& error,
                                                                               std::size_t bytes_transferred) mutable {
                                   on_write(std::move(handler), false, error, bytes_transferred);
                                 }));
  } else {
    boost::asio::async_write(socket_, util::buffer::asio_dynamic_buffer_v1(output_),
                             boost::asio::bind_executor(
                                 strand_, [this, handler = std::move(handler)](const boost::system::error_code& error,
                                                                               std::size_t bytes_transferred) mutable {
                                   on_write(std::move(handler), false, error, bytes_transferred);
                                 }));
  }
}

void base_connection::write_untimed_async(io_callback_t handler) {
  set_writing();
  if (tls_established_) {
    boost::asio::async_write(*secure_socket_, util::buffer::asio_dynamic_buffer_v1(output_),
                             boost::asio::bind_executor(
                                 strand_, [this, handler = std::move(handler)](const boost::system::error_code& error,
                                                                               std::size_t bytes_transferred) mutable {
                                   on_write(std::move(handler), true, error, bytes_transferred);
                                 }));
  } else {
    boost::asio::async_write(socket_, util::buffer::asio_dynamic_buffer_v1(output_),
                             boost::asio::bind_executor(
                                 strand_, [this, handler = std::move(handler)](const boost::system::error_code& error,
                                                                               std::size_t bytes_transferred) mutable {
                                   on_write(std::move(handler), true, error, bytes_transferred);
                                 }));
  }
}

void base_connection::on_write(io_callback_t handler, bool untimed, const boost::system::error_code& error,
                               std::size_t bytes_transferred) {
  finish_writing();
  if (!untimed) {
    timeout_.cancel_timeout();
  }
  if (error != boost::system::errc::success) {
    set_connected(false);
  }
  boost::asio::post(
      ioc_, [handler = std::move(handler), error, bytes_transferred]() mutable { handler(error, bytes_transferred); });
}

void base_connection::on_timeout() {
  timeout_.cancel_timeout();
  if (can_be_shutdown()) {
    shutdown();
  }
  finish_reading();
  finish_writing();
}

void base_connection::shutdown() {
  if (!is_open()) {
    out::safe_warn::log("Shutdown called on an unopened socket");
  } else if (!connected()) {
    out::safe_warn::log("Shutdown called on an unconnected socket");
  }

  out::safe_debug::log("Shutdown on connection", this);
  boost::system::error_code error;
  socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
  set_connected(false);

  switch (error.value()) {
    // Ignore these error values, as they are not fatal.
    case boost::system::errc::success:
    case boost::asio::error::not_connected:
      break;
    default:
      out::safe_error::stream("Error calling shutdown on socket: ", error.message(), " (", error.to_string(), ')',
                              out::manip::endl);
  }
}

void base_connection::close() { socket_.close(); }

void base_connection::disconnect() {
  // Cancel any pending timeouts.
  timeout_.cancel_timeout();
  if (can_be_shutdown()) {
    shutdown();
  }
  close();
}

base_connection& base_connection::operator<<(const byte_array_t& data) {
  std::copy(data.begin(), data.end(), std::ostreambuf_iterator<char>(&output_));
  return *this;
}

}  // namespace proxy::connection
