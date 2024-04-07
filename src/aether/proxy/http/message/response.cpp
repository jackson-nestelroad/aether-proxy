/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "response.hpp"

#include <initializer_list>
#include <iostream>
#include <string_view>

#include "aether/proxy/http/message/status.hpp"
#include "aether/proxy/http/state/cookie_collection.hpp"

namespace proxy::http {

response::response() : status_(status::ok) {}

response::response(http::version version, http::status status_code, std::initializer_list<header_pair_t> headers,
                   std::string content)
    : message(version, headers, std::move(content)), status_(status_code) {}

bool response::is_1xx() const { return static_cast<std::size_t>(status_) / 100 == 1; }

bool response::is_2xx() const { return static_cast<std::size_t>(status_) / 100 == 2; }

bool response::is_3xx() const { return static_cast<std::size_t>(status_) / 100 == 3; }

bool response::is_4xx() const { return static_cast<std::size_t>(status_) / 100 == 4; }

bool response::is_5xx() const { return static_cast<std::size_t>(status_) / 100 == 5; }

bool response::has_cookies() const { return has_header("Set-Cookie"); }

cookie_collection response::get_cookies() const {
  std::vector<std::string> cookie_headers = get_all_of_header("Set-Cookie");
  cookie_collection cookies;
  for (std::string_view header : cookie_headers) {
    auto parsed = cookie::parse_set_header(header);
    if (parsed.has_value()) {
      cookies.set(parsed.value());
    }
  }
  return cookies;
}

void response::set_cookies(const cookie_collection& cookies) {
  remove_header("Set-Cookie");
  for (const auto& [name, cookie] : cookies) {
    add_header("Set-Cookie", cookie.response_string());
  }
}

std::ostream& operator<<(std::ostream& out, const response& res) {
  out << res.version_ << ' ';
  out << res.status_ << ' ';
  out << status_to_reason(res.status_);
  out << message::CRLF;
  out << static_cast<const message&>(res);
  return out;
}

}  // namespace proxy::http
