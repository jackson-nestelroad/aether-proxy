/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/tcp/http/message/request.hpp>
#include <aether/proxy/tcp/http/message/response.hpp>

namespace proxy::tcp::websocket {
    bool is_handshake(http::request &req);
    bool is_handshake(http::response &res);
}