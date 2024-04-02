/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "opcode.hpp"

#include <iostream>
#include <string_view>

#include "aether/proxy/error/error.hpp"

namespace proxy::websocket {

result<std::string_view> opcode_to_string(opcode o) {
  switch (o) {
#define X(name, num, string) \
  case opcode::name:         \
    return string;
    WEBSOCKET_OPCODES(X)
#undef X
  }

  if (o > opcode::max) {
    return error::websocket::invalid_opcode();
  }
  return "reserved";
}

std::ostream& operator<<(std::ostream& output, opcode o) {
  output << static_cast<std::uint8_t>(o);
  return output;
}

}  // namespace proxy::websocket
