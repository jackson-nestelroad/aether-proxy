/*********************************************

  Copyright (c) Jackson Nestelroad 2023
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <string>
#include <utility>

#include "aether/proxy/error/error_code.hpp"
#include "aether/proxy/error/error_state.hpp"
#include "aether/util/result.hpp"

namespace proxy {

template <typename T>
using result = util::result<T, error::error_state>;

namespace error {

#define GENERATE_ALL_FUNCTIONS_false(name, macro_name, msg, foreach) macro_name##_ERRORS(foreach, name)

#define GENERATE_ALL_FUNCTIONS_true(name, macro_name, msg, foreach) \
  namespace name {                                                  \
  macro_name##_ERRORS(foreach, name)                                \
  }

#define GENERATE_ALL_FUNCTIONS(num, name, macro_name, msg, nested, foreach) \
  GENERATE_ALL_FUNCTIONS_##nested(name, macro_name, msg, foreach)

#define GENERATE_FUNCTION(num, name, msg, base)                                 \
  inline error_state name(std::string message = msg) {                          \
    return proxy_error(errc::name, #base "::" #name ": " + std::move(message)); \
  }

ERROR_CATEGORIES(GENERATE_ALL_FUNCTIONS, GENERATE_FUNCTION)

#undef GENERATE_FUNCTION
#undef GENERATE_ALL_FUNCTIONS
#undef GENERATE_ALL_FUNCTIONS_true
#undef GENERATE_ALL_FUNCTIONS_false

}  // namespace error

}  // namespace proxy
