/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "error_state.hpp"

#include <boost/system/error_code.hpp>
#include <iostream>
#include <string>
#include <string_view>

#include "aether/proxy/error/exceptions.hpp"

namespace proxy::error {

error_state::error_state() noexcept : boost_error_code_(), proxy_error_code_(), message_() {}

void error_state::clear() noexcept {
  proxy_error_code_.clear();
  boost_error_code_.clear();
  message_.clear();
}

void error_state::set_proxy_error(const base_exception& ex) noexcept {
  proxy_error_code_ = ex.error_code();
  message_ = ex.what();
}

std::string_view error_state::get_message_or_proxy() const noexcept {
  return message_.empty() ? proxy_error_code_.message() : message_;
}

std::string error_state::get_message_or_boost() const noexcept {
  return message_.empty() ? boost_error_code_.message() : message_;
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
  if (error.has_message()) {
    out << error.message();
  } else {
    proxy::error_code proxy_error = error.proxy_error();
    boost::system::error_code boost_error = error.boost_error();

    bool has_proxy_error = proxy_error != proxy::errc::success;
    bool has_boost_error = boost_error != boost::system::errc::success;

    if (!has_proxy_error && !has_boost_error) {
      // No error.
      out << proxy_error;
    } else {
      if (has_proxy_error) {
        out << "Proxy: " << proxy_error;

        if (has_boost_error) {
          out << ' ';
        }
      }
      if (has_boost_error) {
        out << "Boost: " << boost_error;
      }
    }
  }

  return out;
}

}  // namespace proxy::error
