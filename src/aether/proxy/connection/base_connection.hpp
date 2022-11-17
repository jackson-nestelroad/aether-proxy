/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

#include "aether/program/options.hpp"
#include "aether/proxy/connection/timeout_service.hpp"
#include "aether/proxy/tls/openssl/ssl_context.hpp"
#include "aether/proxy/tls/x509/certificate.hpp"
#include "aether/proxy/types.hpp"
#include "aether/util/streambuf.hpp"

namespace proxy {
class server_components;
}

namespace proxy::connection {

// Base class for a TCP socket connection.
// Can be thought of as a wrapper around the socket class.
class base_connection {
 public:
  static constexpr std::size_t default_buffer_size = 8192;

  // Enumeration type to represent operation mode.
  // Changes the timeout for socket operations.
  enum class io_mode {
    regular,
    tunnel,
    no_timeout,
  };

  // Enumeration type to represent the state of operations on this connection.
  enum class operation_state {
    // Free means the connection has no pending operations.
    free,
    // Pending means the connection has at least one pending operation.
    pending,
  };

  inline io_mode mode() const { return mode_; }
  inline void set_mode(io_mode new_mode) { mode_ = new_mode; }
  inline bool secured() const { return tls_established_; }

  inline boost::asio::ip::tcp::socket& socket() { return socket_; }
  inline boost::asio::ip::tcp::endpoint endpoint() const { return socket_.remote_endpoint(); }
  inline boost::asio::ip::address address() const { return socket_.remote_endpoint().address(); }
  inline boost::asio::io_context& io_context() { return ioc_; }
  inline tls::x509::certificate cert() const { return cert_; }
  inline std::string_view alpn() const { return alpn_; }

  // Tests if the socket has been closed.
  // Reads produce EOF error.
  bool has_been_closed();

  // Reads from the socket synchronously with a custom buffer size.
  // Calls socket.read_some.
  std::size_t read(std::size_t buffer_size, boost::system::error_code& error);

  // Reads from the socket synchronously.
  // Calls socket.read_some.
  std::size_t read(boost::system::error_code& error);

  // Reads all bytes currently available in the socket.
  // This operation must be non-blocking.
  // Calls boost::asio::read.
  std::size_t read_available(boost::system::error_code& error);

  // Reads from the socket asynchronously with a custom buffer size.
  // Calls socket.async_read_some.
  void read_async(std::size_t buffer_size, io_callback_t handler);

  // Reads from the socket asynchronously.
  // Calls socket.async_read_some.
  void read_async(io_callback_t handler);

  // Reads from the socket asynchronously until the given delimiter is in the buffer.
  // Calls boost::asio::async_read_until.
  void read_until_async(std::string_view delim, io_callback_t handler);

  // Writes to the socket synchronously.
  // This operation must be non-blocking.
  std::size_t write(boost::system::error_code& error);

  // Writes to the socket asynchronously using the output buffer.
  void write_async(io_callback_t handler);

  // Writes to the socket asynchronously using the output buffer.
  // Does not put a timeout on the operation.
  // Only use when a timeout is placed on another concurrent operation.
  void write_untimed_async(io_callback_t handler);

  // Closes the socket.
  void close();

  // Returns the number of bytes that are available to be read without blocking.
  inline std::size_t available_bytes() const { return socket_.available(); }

  // Returns an input stream for reading from the input buffer.
  // The input buffer should first be written to using connection.read_async.
  inline std::istream input_stream() { return std::istream(&input_); }

  // Returns an output stream for writing to the output buffer.
  // The output buffer should then be written to the socket using connection.write_async.
  inline std::ostream output_stream() { return std::ostream(&output_); }

  // Returns a reference to the input buffer.
  // The input buffer should first be written to using connection.read_async.
  inline streambuf& input_buffer() { return input_; }

  // Returns a reference to the output buffer.
  // The output buffer should then be written to the socket using connecion.write_async.
  inline streambuf& output_buffer() { return output_; }

  // Returns the input buffer wrapped as a const buffer.
  inline const_buffer const_input_buffer() const { return input_.data(); }

  // Sends the shutdown signal over the socket.
  void shutdown();

  inline bool operations_pending() {
    return read_state_ == operation_state::pending || write_state_ == operation_state::pending;
  }

  template <typename T>
  base_connection& operator<<(const T& data) {
    std::ostream(&output_) << data;
    return *this;
  }

  // Stream insertion overloads.

  base_connection& operator<<(const byte_array_t& data);

 protected:
  base_connection(boost::asio::io_context& ioc, server_components& components);
  base_connection() = delete;
  ~base_connection();
  base_connection(const base_connection& other) = delete;
  base_connection& operator=(const base_connection& other) = delete;
  base_connection(base_connection&& other) noexcept = delete;
  base_connection& operator=(base_connection&& other) noexcept = delete;

  // Handler for asynchronous operation timeouts.
  void on_timeout();

  // Turns on the timeout service to cancel the socket after timeout_ms.
  // Must call timeout.cancel_timeout() to stop the timer.
  // Calls the handler with a timeout error code if applicable.
  void set_timeout();

  inline void set_reading() { read_state_ = operation_state::pending; }
  inline void finish_reading() { read_state_ = operation_state::free; }
  inline void set_writing() { write_state_ = operation_state::pending; }
  inline void finish_writing() { write_state_ = operation_state::free; }

  // Callback for read_async.
  // Use when commit is called automatically.
  void on_read(io_callback_t handler, const boost::system::error_code& error, std::size_t bytes_transferred);

  // Callback for read_async.
  // Commits the bytes_transferred to the buffer.
  void on_read_need_to_commit(io_callback_t handler, const boost::system::error_code& error,
                              std::size_t bytes_transferred);

  // Callback for write_async.
  void on_write(io_callback_t handler, bool untimed, const boost::system::error_code& error,
                std::size_t bytes_transferred);

  static milliseconds default_timeout;
  static milliseconds default_tunnel_timeout;

  streambuf input_;
  streambuf output_;

  program::options& options_;
  boost::asio::io_context& ioc_;
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;
  boost::asio::ip::tcp::socket socket_;
  timeout_service timeout_;
  io_mode mode_;

  bool tls_established_;
  std::unique_ptr<boost::asio::ssl::context> ssl_context_;
  tls::x509::certificate cert_;
  std::unique_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>> secure_socket_;
  std::string alpn_;

  operation_state read_state_;
  operation_state write_state_;
};

}  // namespace proxy::connection
