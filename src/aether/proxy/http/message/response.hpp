/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <initializer_list>
#include <iostream>
#include <string_view>

#include "aether/proxy/http/message/message.hpp"
#include "aether/proxy/http/message/status.hpp"
#include "aether/proxy/http/state/cookie_collection.hpp"

namespace proxy::http {

// A single HTTP response.
class response : public message {
 public:
  response();
  response(http::version version, http::status status_code, std::initializer_list<header_pair_t> headers,
           std::string content);
  ~response() = default;
  response(const response& other) = default;
  response& operator=(const response& other) = default;
  response(response&& other) noexcept = default;
  response& operator=(response&& other) noexcept = default;

  inline http::status status() const { return status_; }
  inline void set_status(http::status status_code) { status_ = status_code; }

  bool is_1xx() const;
  bool is_2xx() const;
  bool is_3xx() const;
  bool is_4xx() const;
  bool is_5xx() const;

  // Returns if there is one or more Set-Cookie headers.
  bool has_cookies() const;

  // Parses and returns the cookies attached to all Set-Cookie headers.
  cookie_collection get_cookies() const;

  // Adds all of the cookies to their own Set-Cookie header.
  //
  // Any previous cookie headers will be deleted.
  void set_cookies(const cookie_collection& cookies);

 private:
  http::status status_;

  friend std::ostream& operator<<(std::ostream& out, const response& res);
};

}  // namespace proxy::http
