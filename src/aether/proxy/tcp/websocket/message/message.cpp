/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "message.hpp"

#include <string_view>

#include "aether/proxy/tcp/websocket/message/endpoint.hpp"
#include "aether/proxy/tcp/websocket/message/opcode.hpp"

namespace proxy::tcp::websocket {

message::message(opcode type, endpoint origin) : type_(type), origin_(origin), content_(), blocked_(false) {}

message::message(opcode type, endpoint origin, std::string_view content)
    : type_(type), origin_(origin), content_(content), blocked_(false) {}

message::message(endpoint origin, std::string_view content)
    : type_(opcode::text), origin_(origin), content_(content), blocked_(false) {}

}  // namespace proxy::tcp::websocket
