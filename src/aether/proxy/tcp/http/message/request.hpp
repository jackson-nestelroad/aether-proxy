/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <initializer_list>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

#include "aether/proxy/tcp/http/message/message.hpp"
#include "aether/proxy/tcp/http/message/method.hpp"
#include "aether/proxy/tcp/http/message/url.hpp"
#include "aether/proxy/tcp/http/state/cookie_collection.hpp"

namespace proxy::tcp::http {

// A single HTTP request.
class request : public message {
 public:
  request() = default;
  request(http::method met, url target, http::version vers, std::initializer_list<header_pair_t> headers,
          std::string content);
  ~request() = default;
  request(const request& other) = default;
  request& operator=(const request& other) = default;
  request(request&& other) noexcept = default;
  request& operator=(request&& other) noexcept = default;

  // Sets the URL object the request is targeting.
  // Updates the Host header.
  void update_target(url target);

  // Updates the request host, automatically updating the Host header.
  void update_host(std::string_view host);

  // Updates the request host, automatically updating the Host header.
  void update_host(std::string_view host, port_t port);

  // Updates the request host, automatically updating the host header.
  void update_host(url::network_location host);

  // Updates the Origin and Referer headers.
  void update_origin_and_referer(std::string_view origin);

  // Updates the Origin and Referer headers.
  void update_origin_and_referer(const url& origin);

  // Checks if the Cookie header exists.
  bool has_cookies() const;

  // Parses and returns the cookies attached to the Cookie header.
  // Only the first Cookie header is parsed.
  cookie_collection get_cookies() const;

  // Sets the Cookie header, overridding any cookies previously stored.
  void set_cookies(const cookie_collection& cookies);

  std::string request_line_string() const;
  std::string absolute_request_line_string() const;

  inline http::method method() const { return method_; }
  inline void set_method(http::method met) { method_ = met; }

  inline const url& target() const { return target_; }

  // Sets the URL object the request is targeting.
  // Does not update any internal headers.
  inline void set_target(url target) { target_ = std::move(target); }

  inline std::string_view host_name() const { return target_.netloc.host; }
  inline port_t host_port() const { return target_.port_or_default(80); }

 private:
  http::method method_;
  url target_;

  friend std::ostream& operator<<(std::ostream& out, const request& req);
};

}  // namespace proxy::tcp::http
