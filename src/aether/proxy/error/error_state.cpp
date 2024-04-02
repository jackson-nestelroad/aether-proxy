/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "error_state.hpp"

#include <boost/system/error_code.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace proxy::error {

error_state::error_state() noexcept : util::generic_error(""), boost_error_code_(), proxy_error_code_() {}

error_state::error_state(util::generic_error error)
    : util::generic_error(std::move(error)), boost_error_code_(), proxy_error_code_() {}

std::string error_state::message() const noexcept {
  if (has_message()) {
    return message_;
  }

  std::ostringstream strm;
  strm << proxy_error();
  if (has_boost_error()) {
    strm << " (boost: " << boost_error() << ")";
  }
  return strm.str();
}

void error_state::clear() noexcept {
  proxy_error_code_.clear();
  boost_error_code_.clear();
  message_.clear();
}

std::string_view error_state::get_message_or_proxy() const noexcept {
  return message_.empty() ? proxy_error_code_.message() : message_;
}

std::string error_state::get_message_or_boost() const noexcept {
  return message_.empty() ? boost_error_code_.message() : message_;
}

error_state boost_error(boost::system::error_code code) {
  error_state err;
  err.set_boost_error(code);
  return err;
}

error_state boost_error(boost::system::error_code code, std::string message) {
  error_state err;
  err.set_boost_error(code);
  err.set_message(std::move(message));
  return err;
}

error_state proxy_error(error_code code, std::string message) {
  error_state err;
  err.set_proxy_error(code);
  err.set_message(std::move(message));
  return err;
}

std::ostream& operator<<(std::ostream& out, const error_state& error) {
  // Error message overrides anything else stored.
  out << error.message();
  return out;
}

}  // namespace proxy::error
