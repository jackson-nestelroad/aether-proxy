/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "aether/proxy/tcp/http/message/request.hpp"
#include "aether/proxy/tcp/http/message/response.hpp"
#include "aether/proxy/tcp/websocket/handshake/extension_data.hpp"

namespace proxy::tcp::websocket::handshake {

bool is_handshake(const http::request& req);
bool is_handshake(const http::response& res);

std::string_view get_client_key(const http::message& msg);
std::string_view get_server_accept(const http::message& msg);
std::optional<std::string_view> get_protocol(const http::message& msg);
std::vector<extension_data> get_extensions(const http::message& msg);

}  // namespace proxy::tcp::websocket::handshake
