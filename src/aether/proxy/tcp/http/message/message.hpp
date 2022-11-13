/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <initializer_list>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "aether/proxy/tcp/http/message/version.hpp"
#include "aether/proxy/types.hpp"
#include "aether/util/string.hpp"

namespace proxy::tcp::http {

// A base class for a single HTTP message.
// Provides common functionality for request and response classes.
class message {
 public:
  static constexpr std::string_view CRLF = "\r\n";
  static constexpr std::string_view CRLF_CRLF = "\r\n\r\n";
  static constexpr char SP = ' ';

  using header_pair_t = std::pair<const std::string, std::string>;
  using headers_map_t = std::multimap<std::string, std::string, util::string::iless>;

 public:
  message();
  message(version _version, std::initializer_list<header_pair_t> headers, std::string body);
  ~message() = default;
  message(const message& other) = default;
  message& operator=(const message& other) = default;
  message(message&& other) noexcept = default;
  message& operator=(message&& other) noexcept = default;

  inline http::version version() const { return version_; }
  inline void set_version(http::version vers) { version_ = vers; }
  inline std::string_view body() const { return body_; }
  inline void set_body(std::string body) { body_ = std::move(body); }
  inline std::size_t content_length() const { return body_.length(); }
  inline const headers_map_t& all_headers() const { return headers_; }

  void add_header(std::string_view name, std::string_view value = "");

  // Sets a header to a single value.
  // All previous headers of the same name are removed.
  void set_header_to_value(std::string_view name, std::string_view value);

  // Removes all values for the given header.
  void remove_header(std::string_view name);

  bool has_header(std::string_view name) const;

  // Checks if header has any value except an empty string.
  bool header_is_nonempty(std::string_view name) const;

  // Checks if a header was given the value exactly.
  bool header_has_value(std::string_view name, std::string_view value, bool case_insensitive = false) const;

  // Checks if a header was given the value in a comma-separated list.
  bool header_has_token(std::string_view name, std::string_view value, bool case_insensitive = false) const;

  // Gets the first value for a given header, throwing if it does not exist.
  // Since headers can be duplicated, it is safer to use get_all_of_header.
  std::string_view get_header(std::string_view name) const;

  // Gets the first value for an optional header.
  std::optional<std::string_view> get_optional_header(std::string_view name) const;

  // Returns a vector of all the values for a given header.
  // Will be empty if header does not exist.
  std::vector<std::string> get_all_of_header(std::string_view name) const;

  // Calculates content length and sets the Content-Length header accordingly.
  // Overwrites previous values.
  void set_content_length();

  bool should_close_connection() const;

 protected:
  http::version version_;
  headers_map_t headers_;
  std::string body_;

  friend std::ostream& operator<<(std::ostream& out, const message& msg);
};

}  // namespace proxy::tcp::http
