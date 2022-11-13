/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>

#define WEBSOCKET_CLOSE_CODES(X)                                    \
  X(1000, normal_closure, "Normal Closure")                         \
  X(1001, going_away, "Going Away")                                 \
  X(1002, protocol_error, "Protocol error")                         \
  X(1003, unsupported_data, "Unsupported Data")                     \
  X(1004, reserved, "Reserved")                                     \
  X(1005, no_status_rcvd, "No Status Rcvd")                         \
  X(1006, abnormal_closure, "Abnormal Closure")                     \
  X(1007, invalid_frame_payload_data, "Invalid frame payload data") \
  X(1008, policy_violation, "Policy Violation")                     \
  X(1009, message_too_big, "Message Too Big")                       \
  X(1010, mandatory_ext, "Mandatory Ext.")                          \
  X(1011, internal_error, "Internal Error")                         \
  X(1012, service_restart, "Service Restart")                       \
  X(1013, try_again_later, "Try Again Later")                       \
  X(1014, bad_gateway, "Bad Gateway")                               \
  X(1015, tls_handshake, "TLS handshake")

namespace proxy::websocket {

// Enumeration type for WebSocket close codes.
enum class close_code {
  min = 1000,
  max = 4999,
#define X(num, name, str) name = num,
  WEBSOCKET_CLOSE_CODES(X)
#undef X
};

// Converts a WebSocket opcode to string.
std::string_view close_code_to_reason(close_code code);

std::ostream& operator<<(std::ostream& output, close_code code);

}  // namespace proxy::websocket
