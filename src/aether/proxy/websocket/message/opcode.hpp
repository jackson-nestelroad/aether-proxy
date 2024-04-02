/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>
#include <string_view>

#include "aether/proxy/error/error.hpp"

#define WEBSOCKET_OPCODES(X)       \
  X(continuation, 0x0, "continue") \
  X(text, 0x1, "text")             \
  X(binary, 0x2, "binary")         \
  X(close, 0x8, "close")           \
  X(ping, 0x9, "ping")             \
  X(pong, 0xA, "pong")             \
  X(max, 0xF, "max")

namespace proxy::websocket {

// Enumeration type for WebSocket opcodes.
enum class opcode {
#define X(name, num, string) name = num,
  WEBSOCKET_OPCODES(X)
#undef X
};

constexpr bool is_control(opcode type) { return static_cast<std::uint8_t>(type) & 0x8; }

// Converts a WebSocket opcode to string.
result<std::string_view> opcode_to_string(opcode o);

std::ostream& operator<<(std::ostream& output, opcode o);

}  // namespace proxy::websocket
