/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

namespace proxy::websocket {

// Data structure representing the three RSV bits in a WebSocket frame.
struct rsv_bits {
  bool rsv1;
  bool rsv2;
  bool rsv3;
};

}  // namespace proxy::websocket
