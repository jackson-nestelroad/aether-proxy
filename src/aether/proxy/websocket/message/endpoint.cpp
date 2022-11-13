/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "endpoint.hpp"

#include <iostream>

namespace proxy::websocket {

std::ostream& operator<<(std::ostream& output, endpoint ep) {
  output << (ep == endpoint::client ? "client" : "server");
  return output;
}

}  // namespace proxy::websocket
