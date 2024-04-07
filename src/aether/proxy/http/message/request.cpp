/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "request.hpp"

#include <initializer_list>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include "aether/proxy/http/message/message.hpp"
#include "aether/proxy/http/message/method.hpp"
#include "aether/proxy/http/message/url.hpp"
#include "aether/proxy/http/state/cookie_collection.hpp"

namespace proxy::http {

request::request(http::method met, url target, http::version version, std::initializer_list<header_pair_t> headers,
                 std::string content)
    : message(version, headers, std::move(content)), method_(met), target_(target) {}

void request::update_target(url target) {
  target_ = std::move(target);
  set_header_to_value("Host", target_.netloc.host);
}

void request::update_host(std::string_view host) {
  target_.netloc.host = host;
  target_.netloc.port = std::nullopt;
  set_header_to_value("Host", target_.netloc.host);
}

void request::update_host(std::string_view host, port_t port) {
  target_.netloc.host = host;
  target_.netloc.port = port;
  set_header_to_value("Host", target_.netloc.host);
}

void request::update_host(url::network_location host) {
  target_.netloc = std::move(host);
  set_header_to_value("Host", target_.netloc.host);
}

void request::update_origin_and_referer(std::string_view origin) {
  set_header_to_value("Origin", origin);
  set_header_to_value("Referer", origin);
}

void request::update_origin_and_referer(const url& origin) {
  set_header_to_value("Origin", origin.origin_string());
  set_header_to_value("Referer", origin.absolute_string());
}

bool request::has_cookies() const { return has_header("Cookie"); }

cookie_collection request::get_cookies() const {
  return cookie_collection::parse_request_header(get_header("Cookie").ok_or(""));
}

void request::set_cookies(const cookie_collection& cookies) { set_header_to_value("Cookie", cookies.request_string()); }

std::string request::request_line_string() const {
  std::stringstream out;
  out << method_ << ' ';
  out << target_ << ' ';
  out << version_;
  return out.str();
}

std::string request::absolute_request_line_string() const {
  std::stringstream out;
  out << method_ << ' ';
  out << target_.absolute_string() << ' ';
  out << version_;
  return out.str();
}

std::ostream& operator<<(std::ostream& out, const request& req) {
  out << req.method_ << ' ';
  out << req.target_ << ' ';
  out << req.version_;
  out << message::CRLF;
  out << static_cast<const message&>(req);
  return out;
}

}  // namespace proxy::http
