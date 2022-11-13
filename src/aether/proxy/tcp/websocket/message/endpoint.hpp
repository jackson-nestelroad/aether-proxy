/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>

namespace proxy::tcp::websocket {

// Enumeration type representing an endpoint of the WebSocket connection.
enum class endpoint { client, server };

constexpr endpoint flip_endpoint(endpoint ep) { return ep == endpoint::client ? endpoint::server : endpoint::client; }

constexpr endpoint operator~(endpoint ep) { return flip_endpoint(ep); }

std::ostream& operator<<(std::ostream& output, endpoint ep);

}  // namespace proxy::tcp::websocket
