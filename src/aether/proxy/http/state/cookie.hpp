/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <locale>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "aether/util/string.hpp"

namespace proxy::http {

// Class representing a single HTTP cookie, which is used to record state across multiple HTTP requests.
class cookie {
 public:
  std::string name;
  std::string value;

  cookie(std::string_view name, std::string_view value);
  cookie() = default;
  ~cookie() = default;
  cookie(const cookie& other) = default;
  cookie& operator=(const cookie& other) = default;
  cookie(cookie&& other) noexcept = default;
  cookie& operator=(cookie&& other) noexcept = default;

  std::optional<std::string> get_attribute(std::string_view attribute) const;
  void set_attribute(std::string_view attribute, std::string_view value);

  std::optional<boost::posix_time::ptime> expires() const;
  std::optional<std::string> domain() const;

  void expire();
  void set_expires(const boost::posix_time::ptime& time);
  void set_domain(std::string_view domain);

  std::string request_string() const;
  std::string response_string() const;

  bool operator<(const cookie& other) const;

  // Parses a cookie from a "Set-Cookie" header.
  //
  // The header string is invalid and should be ignored if a std::nullopt is returned.
  static std::optional<cookie> parse_set_header(std::string_view header);

 private:
  static const std::locale locale;
  static const boost::posix_time::ptime epoch;

  std::map<std::string, std::string, util::string::iless> attributes_;

  friend std::ostream& operator<<(std::ostream& output, const cookie& cookie);
};
}  // namespace proxy::http
