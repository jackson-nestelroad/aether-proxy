/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "close_code.hpp"

#include <iostream>
#include <string_view>

namespace proxy::websocket {

std::string_view close_code_to_reason(close_code code) {
  switch (code) {
#define X(num, name, str) \
  case close_code::name:  \
    return str;
    WEBSOCKET_CLOSE_CODES(X)
#undef X
    default:
      break;
  }
  return "Unknown Close Code";
}

std::ostream& operator<<(std::ostream& output, close_code code) {
  output << static_cast<std::int16_t>(code);
  return output;
}

}  // namespace proxy::websocket
