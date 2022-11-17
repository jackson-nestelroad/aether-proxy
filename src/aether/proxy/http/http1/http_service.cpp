/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "http_service.hpp"

#include <boost/asio.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <string_view>

#include "aether/proxy/connection_handler.hpp"
#include "aether/proxy/constants/server_constants.hpp"
#include "aether/proxy/error/exceptions.hpp"
#include "aether/proxy/server_components.hpp"
#include "aether/proxy/tls/tls_service.hpp"
#include "aether/proxy/tunnel/tunnel_service.hpp"
#include "aether/proxy/websocket/handshake/handshake.hpp"
#include "aether/proxy/websocket/websocket_service.hpp"
#include "aether/util/console.hpp"

namespace proxy::http::http1 {
const response http_service::continue_response = {version::http1_1, status::continue_, {}, ""};
const response http_service::connect_response = {version::http1_1, status::ok, {}, ""};

http_service::http_service(connection::connection_flow& flow, connection_handler& owner, server_components& components)
    : base_service(flow, owner, components), exchange_(), parser_(exchange_, components) {}

void http_service::start() {
  // Errors may be stored up to be sent over HTTP.
  if (flow_.error.has_proxy_error()) {
    send_error_response(status::bad_gateway, flow_.error.get_message_or_proxy());
  } else {
    // Client connection has the initial HTTP request.
    // We read the request headers and then handle it accordingly.
    read_request_head();
  }
}

void http_service::read_request_head() {
  // Read until the end of headers delimiter is found.
  flow_.client.read_until_async(message::CRLF_CRLF, std::bind_front(&http_service::on_read_request_head, this));
}

void http_service::on_read_request_head(const boost::system::error_code& error, std::size_t bytes_transferred) {
  if (error != boost::system::errc::success) {
    // No new request started.
    if (bytes_transferred == 0) {
      stop();
      return;
    }

    flow_.error.set_boost_error(error);
    if (error == boost::asio::error::operation_aborted) {
      send_error_response(status::request_timeout, error.message());
    } else {
      send_error_response(status::bad_request, error.message());
    }
  } else {
    try {
      std::istream input = flow_.client.input_stream();
      parser_.read_request_line(input);
      parser_.read_headers(input, http_parser::message_mode::request);
      read_request_body(std::bind_front(&http_service::handle_request, this));
    } catch (const error::base_exception& ex) {
      flow_.error.set_proxy_error(ex);
      send_error_response(status::bad_request, ex.what());
    }
  }
}

void http_service::read_request_body(callback_t handler) {
  // Unlike the headers, finding the end of the body is a bit tricky.
  // The client's input buffer may not have all of the body in it.
  // The flow for reading a HTTP body is the following:
  //  1. Figure out how to read the body (Content-Length, chunked encoding, or until end of stream).
  //  2. Read/parse whatever is in the input buffer.
  //  3. Store body parsing state internally.
  //  4. If the body is not complete, read from the socket and return to step 2.
  //  5. Body is complete, call handler.
  try {
    // Parse what we have, or what we just read.
    std::istream input = flow_.client.input_stream();

    // Need more data from the socket
    if (!parser_.read_body(input, http_parser::message_mode::request)) {
      flow_.client.read_async([this, handler = std::move(handler)](const boost::system::error_code& error,
                                                                   std::size_t bytes_transferred) mutable {
        on_read_request_body(std::move(handler), error, bytes_transferred);
      });
    } else {
      // Body is finished, call finished handler.
      handler();
    }
  } catch (const error::base_exception& ex) {
    flow_.error.set_proxy_error(ex);
    send_error_response(status::bad_request, ex.what());
  }
}

void http_service::on_read_request_body(callback_t handler, const boost::system::error_code& error,
                                        std::size_t bytes_transferred) {
  if (error != boost::system::errc::success) {
    flow_.error.set_boost_error(error);
    if (error == boost::asio::error::operation_aborted) {
      send_error_response(status::request_timeout, error.message());
    } else {
      send_error_response(status::bad_request, error.message());
    }
  } else {
    // Just go back to reading/parsing the body.
    read_request_body(std::move(handler));
  }
}

void http_service::handle_request() {
  try {
    request& req = exchange_.request();

    validate_target();

    interceptors_.http.run(intercept::http_event::any_request, flow_, exchange_);

    // Insert Via header.
    req.add_header("Via", out::string::stream("1.1 ", constants::lowercase_name));

    // This is a CONNECT request.
    if (req.target().form == url::target_form::authority) {
      interceptors_.http.run(intercept::http_event::connect, flow_, exchange_);

      set_server(std::string(req.host_name()), req.host_port());

      // An interceptor may set a response.
      // If it does not, use the default 200 response.
      if (!exchange_.has_response()) {
        exchange_.set_response(connect_response);
      }

      send_connect_response();
      return;
    }

    if (req.header_has_value("Expect", "100-continue")) {
      flow_.client << continue_response;
      // Write small response synchronously.
      boost::system::error_code write_err;
      flow_.client.write(write_err);
      if (write_err != boost::system::errc::success) {
        stop();
        return;
      }
      req.remove_header("Expect");
    }

    interceptors_.http.run(intercept::http_event::request, flow_, exchange_);
    set_server(std::string(req.host_name()), req.host_port());

    // Response set by an interceptor.
    if (exchange_.has_response()) {
      forward_response();
    } else {
      // Must get the response from the server.
      if (websocket::handshake::is_handshake(req)) {
        interceptors_.http.run(intercept::http_event::websocket_handshake, flow_, exchange_);
      }
      connect_server();
    }
  } catch (const error::base_exception& ex) {
    flow_.error.set_proxy_error(ex);
    send_error_response(status::bad_request, ex.what());
  }
}

void http_service::validate_target() {
  request& req = exchange_.request();
  url target = req.target();

  // Make sure target URL and host header are OK to be forwarded.
  // This form is when the client knows it is talking to a proxy.
  if (target.form == url::target_form::absolute) {
    // Add missing host header.
    if (!req.has_header("Host")) {
      req.set_header_to_value("Host", target.netloc.to_host_string());
    }

    // Convert absolute form to origin form to forward the request.
    // This is not technically required, but it's safer in case the server is expecting only an origin request.
    target.form = url::target_form::origin;
  } else if (target.form == url::target_form::origin && !target.netloc.has_hostname()) {
    // This form occurs when the client does not know it is talking to the proxy, so it sends an origin-form request.
    if (!req.has_header("Host")) {
      // No host information in target, so we need to parse it and set it to keep things uniform.
      // Absolutely no way to get the intended host.
      send_error_response(status::bad_request, "No host given.");
      return;
    }
    // Parse host header, and set the host.
    target.netloc = url::parse_netloc(req.get_header("Host"));
  }

  // Set scheme based on server connection.
  if (target.form != url::target_form::authority && target.scheme.empty()) {
    target.scheme = flow_.server.secured() ? "https" : "http";
  }

  // Set default port.
  if (!target.netloc.has_port()) {
    target.netloc.port = (target.scheme == "https" || flow_.server.secured()) ? 443 : 80;
  }

  req.set_target(target);
}

void http_service::connect_server() { connect_server_async(std::bind_front(&http_service::on_connect_server, this)); }

void http_service::on_connect_server(const boost::system::error_code& error) {
  if (error != boost::system::errc::success) {
    flow_.error.set_boost_error(error);
    if (error == boost::asio::error::operation_aborted) {
      send_error_response(status::gateway_timeout, error.message());
    } else {
      send_error_response(status::bad_gateway, error.message());
    }
  } else {
    forward_request();
  }
}

void http_service::forward_request() {
  flow_.server << exchange_.request();
  flow_.server.write_async(std::bind_front(&http_service::on_forward_request, this));
}

void http_service::on_forward_request(const boost::system::error_code& error, std::size_t bytes_transferred) {
  if (error != boost::system::errc::success) {
    flow_.error.set_boost_error(error);
    if (error == boost::asio::error::operation_aborted) {
      send_error_response(status::gateway_timeout, error.message());
    } else {
      send_error_response(status::internal_server_error, error.message());
    }
  } else {
    read_response_head();
  }
}

void http_service::read_response_head() {
  // Read until the end of headers delimiter is found.
  flow_.server.read_until_async(message::CRLF_CRLF, std::bind_front(&http_service::on_read_response_head, this));
}

void http_service::on_read_response_head(const boost::system::error_code& error, std::size_t bytes_transferred) {
  if (error != boost::system::errc::success) {
    flow_.error.set_boost_error(error);
    if (error == boost::asio::error::operation_aborted) {
      send_error_response(status::gateway_timeout, error.message());
    } else {
      // TODO: bad record type (SSL routines) error dumps its error here.
      send_error_response(status::internal_server_error, error.message());
    }
  } else {
    std::istream input = flow_.server.input_stream();
    exchange_.make_response();
    parser_.read_response_line(input);
    parser_.read_headers(input, http_parser::message_mode::response);
    read_response_body(std::bind_front(&http_service::forward_response, this));
  }
}

void http_service::read_response_body(callback_t handler, bool eof) {
  // Just like read_request_body.
  try {
    // Parse what we have, or what we just read.
    std::istream input = flow_.server.input_stream();

    if (parser_.read_body(input, http_parser::message_mode::response)) {
      // Body is finished, call finished handler.
      handler();
    } else if (eof) {
      // Body is not finished, and nothing more to read.
      flow_.error.set_boost_error(boost::asio::error::eof);
      flow_.error.set_proxy_error(errc::malformed_response_body);
      send_error_response(status::internal_server_error, flow_.error.proxy_error().message());
    } else {
      // Need more data from the socket.
      flow_.server.read_async([this, handler = std::move(handler)](const boost::system::error_code& error,
                                                                   std::size_t bytes_transferred) mutable {
        on_read_response_body(std::move(handler), error, bytes_transferred);
      });
    }
  } catch (const error::base_exception& ex) {
    flow_.error.set_proxy_error(ex);
    send_error_response(status::internal_server_error, ex.what());
  }
}

void http_service::on_read_response_body(callback_t handler, const boost::system::error_code& error,
                                         std::size_t bytes_transferred) {
  if (error != boost::system::errc::success) {
    // Connection was closed by the server.
    // This may be desired if reading body until end of stream.
    if (error == boost::asio::error::eof) {
      read_response_body(std::move(handler), true);
    } else {
      flow_.error.set_boost_error(error);
      if (error == boost::asio::error::operation_aborted) {
        send_error_response(status::gateway_timeout, error.message());
      } else {
        send_error_response(status::internal_server_error, error.message());
      }
    }
  } else {
    read_response_body(std::move(handler), bytes_transferred == 0);
  }
}

void http_service::forward_response() {
  interceptors_.http.run(intercept::http_event::response, flow_, exchange_);

  flow_.client << exchange_.response();
  flow_.client.write_async(std::bind_front(&http_service::on_forward_response, this));
}
void http_service::on_forward_response(const boost::system::error_code& error, std::size_t bytes_transferred) {
  if (error != boost::system::errc::success) {
    flow_.error.set_boost_error(error);
    stop();
  } else {
    handle_response();
  }
}

void http_service::handle_response() {
  bool should_close = exchange_.request().should_close_connection() || exchange_.response().should_close_connection();
  if (should_close) {
    stop();
    return;
  }

  if (exchange_.response().status() == status::switching_protocols) {
    if (!options_.websocket_passthrough_strict &&
        (!options_.websocket_passthrough || flow_.should_intercept_websocket()) &&
        websocket::handshake::is_handshake(exchange_.request()) &&
        websocket::handshake::is_handshake(exchange_.response())) {
      owner_.switch_service<websocket::websocket_service>(exchange_);
    } else {
      owner_.switch_service<tunnel::tunnel_service>();
    }
  } else {
    owner_.switch_service<http_service>();
  }
}

void http_service::send_connect_response() {
  flow_.client << exchange_.response();
  flow_.client.write_async(std::bind_front(&http_service::on_send_connect_response, this));
}

void http_service::on_send_connect_response(const boost::system::error_code& error, std::size_t bytes_transferred) {
  if (exchange_.response().is_2xx()) {
    // Interceptors can mask a CONNECT request and treat it like a normal request.
    if (exchange_.mask_connect()) {
      owner_.switch_service<http_service>();
    } else if (options_.ssl_passthrough_strict || (options_.ssl_passthrough && !flow_.should_intercept_tls())) {
      // In strict passthrough mode, use tunnel by default.
      // In passthrough mode, use tunnel if not marked by a CONNECT interceptor.
      owner_.switch_service<tunnel::tunnel_service>();
    } else {
      // Default, use TLS service.
      // TLS may not be the correct option.
      // If it is not, the stream will be forwarded to the TCP tunnel service.
      owner_.switch_service<tls::tls_service>();
    }
  } else if (exchange_.response().is_3xx()) {
    owner_.switch_service<http_service>();
  } else {
    stop();
  }
}

void http_service::send_error_response(status response_status, std::string_view msg) {
  response& res = exchange_.make_response();
  res.set_status(response_status);

  std::string_view reason = status_to_reason(response_status);

  // A small hint of server-side rendering
  std::stringstream content;
  content << "<html><head>";
  content << "<title>" << response_status << ' ' << reason << "</title>";
  content << "</head><body>";
  content << "<h1>" << response_status << ' ' << reason << "</h1>";
  content << "<p>" << msg << "</p>";
  content << "</body></html>";
  res.set_body(content.str());

  res.add_header("Server", constants::full_server_name.data());
  res.add_header("Connection", "close");
  res.add_header("Content-Type", "text/html");
  res.set_content_length();

  flow_.client << res;
  flow_.client.write_async(std::bind_front(&http_service::on_write_error_response, this));
}

void http_service::on_write_error_response(const boost::system::error_code& err, std::size_t bytes_transferred) {
  // No reason to check errors here.
  interceptors_.http.run(intercept::http_event::error, flow_, exchange_);
  stop();
}

}  // namespace proxy::http::http1
