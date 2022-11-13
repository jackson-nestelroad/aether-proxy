/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "base_connection.hpp"

#include <algorithm>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>

#include "aether/proxy/connection/timeout_service.hpp"
#include "aether/proxy/server_components.hpp"
#include "aether/proxy/tcp/tls/openssl/ssl_context.hpp"
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
      tls_established_(false),
      ssl_context_(),
      cert_(nullptr),
      secure_socket_() {}

base_connection::~base_connection() {}

void base_connection::set_timeout() {
  switch (mode_) {
    case io_mode::regular:
      timeout_.set_timeout(options_.timeout, boost::bind(&base_connection::on_timeout, this));
      return;
    case io_mode::tunnel:
      timeout_.set_timeout(options_.tunnel_timeout, boost::bind(&base_connection::on_timeout, this));
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
  set_timeout();

  std::size_t bytes_read = 0;
  if (tls_established_) {
    bytes_read = secure_socket_->read_some(input_.prepare_sequence(buffer_size), error);
  } else {
    bytes_read = socket_.read_some(input_.prepare_sequence(buffer_size), error);
  }

  input_.commit(bytes_read);
  timeout_.cancel_timeout();
  return bytes_read;
}

std::size_t base_connection::read_available(boost::system::error_code& error) {
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
  return bytes_read;
}

void base_connection::read_async(const io_callback_t& handler) { read_async(default_buffer_size, handler); }

void base_connection::read_async(std::size_t buffer_size, const io_callback_t& handler) {
  set_timeout();
  if (tls_established_) {
    secure_socket_->async_read_some(
        input_.prepare_sequence(buffer_size),
        boost::asio::bind_executor(
            strand_, boost::bind(&base_connection::on_read_need_to_commit, shared_from_this(), handler,
                                 boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
  } else {
    socket_.async_read_some(
        input_.prepare_sequence(buffer_size),
        boost::asio::bind_executor(
            strand_, boost::bind(&base_connection::on_read_need_to_commit, shared_from_this(), handler,
                                 boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
  }
}

void base_connection::read_until_async(std::string_view delim, const io_callback_t& handler) {
  set_timeout();
  if (tls_established_) {
    boost::asio::async_read_until(
        *secure_socket_, util::buffer::asio_dynamic_buffer_v1(input_), delim,
        boost::asio::bind_executor(
            strand_, boost::bind(&base_connection::on_read, shared_from_this(), handler,
                                 boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
  } else {
    boost::asio::async_read_until(
        socket_, util::buffer::asio_dynamic_buffer_v1(input_), delim,
        boost::asio::bind_executor(
            strand_, boost::bind(&base_connection::on_read, shared_from_this(), handler,
                                 boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
  }
}

void base_connection::on_read(const io_callback_t& handler, const boost::system::error_code& error,
                              std::size_t bytes_transferred) {
  timeout_.cancel_timeout();
  boost::asio::post(ioc_, boost::bind(handler, error, bytes_transferred));
}

void base_connection::on_read_need_to_commit(const io_callback_t& handler, const boost::system::error_code& error,
                                             std::size_t bytes_transferred) {
  input_.commit(bytes_transferred);
  on_read(handler, error, bytes_transferred);
}

std::size_t base_connection::write(boost::system::error_code& error) {
  set_timeout();

  std::size_t bytes_written = 0;
  if (tls_established_) {
    bytes_written = boost::asio::write(*secure_socket_, util::buffer::asio_dynamic_buffer_v1(output_), error);
  } else {
    bytes_written = boost::asio::write(socket_, util::buffer::asio_dynamic_buffer_v1(output_), error);
  }

  timeout_.cancel_timeout();
  return bytes_written;
}

void base_connection::write_async(const io_callback_t& handler) {
  set_timeout();
  if (tls_established_) {
    boost::asio::async_write(
        *secure_socket_, util::buffer::asio_dynamic_buffer_v1(output_),
        boost::asio::bind_executor(
            strand_, boost::bind(&base_connection::on_write, shared_from_this(), handler,
                                 boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
  } else {
    boost::asio::async_write(
        socket_, util::buffer::asio_dynamic_buffer_v1(output_),
        boost::asio::bind_executor(
            strand_, boost::bind(&base_connection::on_write, shared_from_this(), handler,
                                 boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
  }
}

void base_connection::write_untimed_async(const io_callback_t& handler) {
  if (tls_established_) {
    boost::asio::async_write(
        *secure_socket_, util::buffer::asio_dynamic_buffer_v1(output_),
        boost::asio::bind_executor(
            strand_, boost::bind(&base_connection::on_untimed_write, shared_from_this(), handler,
                                 boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
  } else {
    boost::asio::async_write(
        socket_, util::buffer::asio_dynamic_buffer_v1(output_),
        boost::asio::bind_executor(
            strand_, boost::bind(&base_connection::on_untimed_write, shared_from_this(), handler,
                                 boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
  }
}

void base_connection::on_write(const io_callback_t& handler, const boost::system::error_code& error,
                               std::size_t bytes_transferred) {
  timeout_.cancel_timeout();
  boost::asio::post(ioc_, boost::bind(handler, error, bytes_transferred));
}

void base_connection::on_untimed_write(const io_callback_t& handler, const boost::system::error_code& error,
                                       std::size_t bytes_transferred) {
  boost::asio::post(ioc_, boost::bind(handler, error, bytes_transferred));
}

void base_connection::shutdown() { socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both); }

void base_connection::on_timeout() {
  // TODO: Maybe ignore the error here altogether? Timeout usually means we're finished anyway.
  boost::system::error_code error;
  socket_.cancel(error);
  switch (error.value()) {
    case boost::system::errc::success:
    case boost::system::errc::no_such_device_or_address:
    case boost::asio::error::bad_descriptor:
      break;
    default:
      throw error::asio_error_exception{error.message()};
  }
}

void base_connection::cancel(boost::system::error_code& error) { socket_.cancel(error); }

void base_connection::close() {
  // shutdown();
  // Cancel any pending timeouts.
  timeout_.cancel_timeout();
  socket_.close();
}

base_connection& base_connection::operator<<(const byte_array_t& data) {
  std::copy(data.begin(), data.end(), std::ostreambuf_iterator<char>(&output_));
  return *this;
}

}  // namespace proxy::connection
