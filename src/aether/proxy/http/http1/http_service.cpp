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
#include "aether/proxy/error/error.hpp"
#include "aether/proxy/server_components.hpp"
#include "aether/proxy/tls/tls_service.hpp"
#include "aether/proxy/tunnel/tunnel_service.hpp"
#include "aether/proxy/websocket/handshake/handshake.hpp"
#include "aether/proxy/websocket/websocket_service.hpp"
#include "aether/util/console.hpp"
#include "aether/util/result_macros.hpp"

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
    std::istream input = flow_.client.input_stream();
    if (result<void> res = parser_.read_request_line(input); !res.is_ok()) {
      flow_.error = res.err();
      send_error_response(status::bad_request, res.err().message());
      return;
    }
    if (result<void> res = parser_.read_headers(input, http_parser::message_mode::request); !res.is_ok()) {
      flow_.error = res.err();
      send_error_response(status::bad_request, res.err().message());
      return;
    }
    read_request_body(std::bind_front(&http_service::handle_request, this));
  }
}

void http_service::read_request_body(callback_t handler) {
  if (result<void> res = read_request_body_impl(std::move(handler)); !res.is_ok()) {
    flow_.error = std::move(res).err();
    send_error_response(status::bad_request, flow_.error.message());
  }
}

result<void> http_service::read_request_body_impl(callback_t handler) {
  // Unlike the headers, finding the end of the body is a bit tricky.
  // The client's input buffer may not have all of the body in it.
  // The flow for reading a HTTP body is the following:
  //  1. Figure out how to read the body (Content-Length, chunked encoding, or until end of stream).
  //  2. Read/parse whatever is in the input buffer.
  //  3. Store body parsing state internally.
  //  4. If the body is not complete, read from the socket and return to step 2.
  //  5. Body is complete, call handler.
  // Parse what we have, or what we just read.
  std::istream input = flow_.client.input_stream();

  // Need more data from the socket
  ASSIGN_OR_RETURN(bool done, parser_.read_body(input, http_parser::message_mode::request));
  if (!done) {
    flow_.client.read_async([this, handler = std::move(handler)](const boost::system::error_code& error,
                                                                 std::size_t bytes_transferred) mutable {
      on_read_request_body(std::move(handler), error, bytes_transferred);
    });
  } else {
    // Body is finished, call finished handler.
    handler();
  }
  return util::ok;
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
  if (result<void> res = handle_request_impl(); !res.is_ok()) {
    flow_.error = std::move(res).err();
    send_error_response(status::bad_request, flow_.error.message());
  }
}

result<void> http_service::handle_request_impl() {
  request& req = exchange_.request();

  RETURN_IF_ERROR(validate_target());

  interceptors_.http.run(intercept::http_event::any_request, flow_, exchange_);

  // Insert Via header.
  req.add_header("Via", out::string::stream("1.1 ", constants::lowercase_name));

  // This is a CONNECT request.
  if (req.target().form == url::target_form::authority) {
    interceptors_.http.run(intercept::http_event::connect, flow_, exchange_);

    RETURN_IF_ERROR(set_server(std::string(req.host_name()), req.host_port()));

    // An interceptor may set a response.
    // If it does not, use the default 200 response.
    if (!exchange_.has_response()) {
      exchange_.set_response(connect_response);
    }

    send_connect_response();
    return util::ok;
  }

  if (req.header_has_value("Expect", "100-continue")) {
    flow_.client << continue_response;
    // Write small response synchronously.
    boost::system::error_code write_err;
    flow_.client.write(write_err);
    if (write_err != boost::system::errc::success) {
      stop();
      return util::ok;
    }
    req.remove_header("Expect");
    return util::ok;
  }

  interceptors_.http.run(intercept::http_event::request, flow_, exchange_);
  RETURN_IF_ERROR(set_server(std::string(req.host_name()), req.host_port()));

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
  return util::ok;
}

result<void> http_service::validate_target() {
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
      return error::http::invalid_target_host("No host given.");
    }
    // Parse host header, and set the host.
    ASSIGN_OR_RETURN(std::string_view host_header, req.get_header("Host"));
    target.netloc = url::parse_netloc(host_header);
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
  return util::ok;
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
  if (result<void> res = on_read_response_head_impl(error, bytes_transferred); !res.is_ok()) {
    flow_.error = std::move(res).err();
    send_error_response(status::internal_server_error, flow_.error.message());
  }
}

result<void> http_service::on_read_response_head_impl(const boost::system::error_code& error,
                                                      std::size_t bytes_transferred) {
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
    RETURN_IF_ERROR(parser_.read_response_line(input));
    RETURN_IF_ERROR(parser_.read_headers(input, http_parser::message_mode::response));
    read_response_body(std::bind_front(&http_service::modify_response, this));
  }
  return util::ok;
}

void http_service::read_response_body(callback_t handler, bool eof) {
  if (result<void> res = read_response_body_impl(std::move(handler), eof); !res.is_ok()) {
    flow_.error = std::move(res).err();
    send_error_response(status::internal_server_error, flow_.error.message());
  }
}

result<void> http_service::read_response_body_impl(callback_t handler, bool eof) {
  // Just like read_request_body.
  // Parse what we have, or what we just read.
  std::istream input = flow_.server.input_stream();

  ASSIGN_OR_RETURN(bool done, parser_.read_body(input, http_parser::message_mode::response));
  if (done) {
    // Body is finished, call finished handler.
    handler();
  } else if (eof) {
    // Body is not finished, and nothing more to read.
    error::error_state err;
    err.set_boost_error(boost::asio::error::eof);
    err.set_proxy_error(errc::malformed_response_body);
    return err;
  } else {
    // Need more data from the socket.
    flow_.server.read_async([this, handler = std::move(handler)](const boost::system::error_code& error,
                                                                 std::size_t bytes_transferred) mutable {
      on_read_response_body(std::move(handler), error, bytes_transferred);
    });
  }
  return util::ok;
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

void http_service::modify_response() {
  exchange_.response().set_header_to_value(out::string::stream(proxy::constants::server_name, "-Connection-Id"),
                                           flow_.id().to_string());
  interceptors_.http.run(intercept::http_event::response, flow_, exchange_);
  forward_response();
}

void http_service::forward_response() {
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
    // TODO: Set client or server to disconnected.
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

  std::string_view reason = "Unknown status code";
  if (result<std::string_view> res = status_to_reason(response_status); res.is_ok()) {
    reason = std::move(res).ok();
  }

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
