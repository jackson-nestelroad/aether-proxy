/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>
#include <string_view>

#include "aether/proxy/base_service.hpp"
#include "aether/proxy/connection/connection_flow.hpp"
#include "aether/proxy/http/exchange.hpp"
#include "aether/proxy/http/http1/http_parser.hpp"
#include "aether/proxy/types.hpp"

namespace proxy::http::http1 {

// Service for handling HTTP/1.x connections.
class http_service : public base_service {
 public:
  http_service(connection::connection_flow& flow, connection_handler& owner, server_components& components);
  void start() override;
  inline http::exchange& exchange() { return exchange_; }

 private:
  // Methods are quite split up because socket operations are asynchronous.

  void read_request_head();
  void on_read_request_head(const boost::system::error_code& error, std::size_t bytes_transferred);
  void read_request_body(callback_t handler);
  result<void> read_request_body_impl(callback_t handler);
  void on_read_request_body(callback_t handler, const boost::system::error_code& error, std::size_t bytes_transferred);
  void handle_request();
  result<void> handle_request_impl();
  void connect_server();
  void on_connect_server(const boost::system::error_code& error);
  void forward_request();
  void on_forward_request(const boost::system::error_code& error, std::size_t bytes_transferred);
  void read_response_head();
  void on_read_response_head(const boost::system::error_code& error, std::size_t bytes_transferred);
  result<void> on_read_response_head_impl(const boost::system::error_code& error, std::size_t bytes_transferred);
  void read_response_body(callback_t handler, bool eof = false);
  result<void> read_response_body_impl(callback_t handler, bool eof = false);
  void on_read_response_body(callback_t handler, const boost::system::error_code& error, std::size_t bytes_transferred);
  void modify_response();
  void forward_response();
  void on_forward_response(const boost::system::error_code& error, std::size_t bytes_transferred);
  void handle_response();

  void send_connect_response();
  void on_send_connect_response(const boost::system::error_code& error, std::size_t bytes_transferred);

  // Modifies the request target if needed for transmission.
  //
  // Host information is kept primarily in the request.target.netloc field.
  result<void> validate_target();

  // Sends an HTML error response to the client.
  //
  // Overwrites any previous response data in the HTTP exchange.
  void send_error_response(status response_code, std::string_view msg = "No message given");
  void on_write_error_response(const boost::system::error_code& error, std::size_t bytes_transferred);

  // Static continue response used whenever client expects it.
  static const response continue_response;

  // Static response to CONNECT requests used whenever client needs it.
  static const response connect_response;

  http::exchange exchange_;
  http_parser parser_;
};

}  // namespace proxy::http::http1
