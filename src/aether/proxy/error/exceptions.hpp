/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include "aether/proxy/error/error_code.hpp"

namespace proxy::error {

// Base exception class for any error that comes out of the proxy.
class base_exception : public std::runtime_error {
 public:
  explicit base_exception(const std::string& msg, errc::errc_t code) : std::runtime_error(msg), code_(code) {}

  constexpr proxy::error_code error_code() const noexcept { return code_; }

 private:
  errc::errc_t code_;
};

// Generate an exception class for each error category and error code to enable try-catch blocks.
// The proxy category is kept in the proxy::error:: namespace. All other categories get their own namespace.

#define GENERATE_ALL_EXCEPTIONS_false(name, macro_name, msg, foreach) macro_name##_ERRORS(foreach, base_exception)

#define GENERATE_ALL_EXCEPTIONS_true(name, macro_name, msg, foreach)                                \
  class name##_exception : public base_exception {                                                  \
   public:                                                                                          \
    explicit name##_exception(const std::string& str = msg, errc::errc_t code = errc::name##_error) \
        : base_exception(str, code) {}                                                              \
  };                                                                                                \
  namespace name {                                                                                  \
  macro_name##_ERRORS(foreach, name##_exception)                                                    \
  }

#define GENERATE_ALL_EXCEPTIONS(num, name, macro_name, msg, nested, foreach) \
  GENERATE_ALL_EXCEPTIONS_##nested(name, macro_name, msg, foreach)

#define GENERATE_EXCEPTION(num, name, msg, base)                                                            \
  class name##_exception : public base {                                                                    \
   public:                                                                                                  \
    explicit name##_exception(const std::string& str = msg) : base(#name ": " + str, errc::errc_t::name) {} \
  };

ERROR_CATEGORIES(GENERATE_ALL_EXCEPTIONS, GENERATE_EXCEPTION)

#undef GENERATE_EXCEPTION
#undef GENERATE_ALL_EXCEPTIONS
#undef GENERATE_ALL_EXCEPTIONS_true
#undef GENERATE_ALL_EXCEPTIONS_false

}  // namespace proxy::error
