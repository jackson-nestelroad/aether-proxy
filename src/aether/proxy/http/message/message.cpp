/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "message.hpp"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "aether/proxy/error/error.hpp"
#include "aether/proxy/http/message/version.hpp"
#include "aether/proxy/types.hpp"
#include "aether/util/console.hpp"
#include "aether/util/string.hpp"

namespace proxy::http {
message::message() : version_(version::http1_1) {}

message::message(http::version version, std::initializer_list<header_pair_t> headers, std::string body)
    : version_(version), headers_(headers), body_(std::move(body)) {}

void message::add_header(std::string_view name, std::string_view value) { headers_.emplace(name, value); }

void message::set_header_to_value(std::string_view name, std::string_view value) {
  remove_header(name);
  add_header(name, value);
}

void message::remove_header(std::string_view name) {
  auto it = headers_.find(name);
  if (it != headers_.end()) {
    headers_.erase(it);
  }
}

bool message::has_header(std::string_view name) const { return headers_.find(name) != headers_.end(); }

bool message::header_is_nonempty(std::string_view name) const {
  auto equal_range = headers_.equal_range(name);
  return std::all_of(equal_range.first, equal_range.second, [](const auto& pair) { return !pair.second.empty(); });
}

bool message::header_has_value(std::string_view name, std::string_view value, bool case_insensitive) const {
  auto equal_range = headers_.equal_range(name);
  if (case_insensitive) {
    return std::any_of(equal_range.first, equal_range.second,
                       [&value](const auto& pair) { return util::string::iequals_fn(pair.second, value); });
  } else {
    return std::any_of(equal_range.first, equal_range.second,
                       [&value](const auto& pair) { return pair.second == value; });
  }
}

bool message::header_has_token(std::string_view name, std::string_view value, bool case_insensitive) const {
  auto equal_range = headers_.equal_range(name);
  if (case_insensitive) {
    return std::any_of(equal_range.first, equal_range.second, [&value](const auto& pair) {
      std::vector<std::string_view> tokens = util::string::split_trim<std::string_view>(pair.second, ',');
      return std::any_of(tokens.begin(), tokens.end(),
                         [&value](const auto& str) { return util::string::iequals_fn(str, value); });
    });
  } else {
    return std::any_of(equal_range.first, equal_range.second, [&value](const auto& pair) {
      std::vector<std::string_view> tokens = util::string::split_trim<std::string_view>(pair.second, ',');
      return std::any_of(tokens.begin(), tokens.end(), [&value](const auto& str) { return str == value; });
    });
  }
}

result<std::string_view> message::get_header(std::string_view name) const {
  auto it = headers_.find(name);
  if (it == headers_.end()) {
    return error::http::header_not_found(out::string::stream("Header \"", name, "\" does not exist"));
  }
  return it->second;
}

std::optional<std::string_view> message::get_optional_header(std::string_view name) const {
  auto it = headers_.find(name);
  return it == headers_.end() ? std::optional<std::string_view>{} : it->second;
}

std::vector<std::string> message::get_all_of_header(std::string_view name) const {
  auto equal_range = headers_.equal_range(name);
  std::vector<std::string> out;
  std::transform(equal_range.first, equal_range.second, std::back_inserter(out),
                 [](const auto& it) { return it.second; });
  return out;
}

void message::set_content_length() { add_header("Content-Length", boost::lexical_cast<std::string>(content_length())); }

bool message::should_close_connection() const {
  if (has_header("Connection")) {
    result<std::string_view> connection_header = get_header("Connection");
    if (connection_header.is_ok()) {
      if (connection_header.ok() == "keep-alive") {
        return false;
      }
      if (connection_header.ok() == "close") {
        return true;
      }
    }
  }
  return version_ == version::http1_0;
}

std::ostream& operator<<(std::ostream& out, const message& msg) {
  for (const auto& [name, value] : msg.headers_) {
    out << name << ": " << value << message::CRLF;
  }
  out << message::CRLF;

  if (msg.header_has_token("Transfer-Encoding", "chunked")) {
    out << std::hex << msg.body_.length() << message::CRLF;
    out << msg.body_;
    out << message::CRLF;
    out << '0' << message::CRLF_CRLF;
  } else {
    out << msg.body_;
  }
  return out;
}

}  // namespace proxy::http
