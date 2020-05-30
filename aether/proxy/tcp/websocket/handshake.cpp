/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "handshake.hpp"

namespace proxy::tcp::websocket {
    bool is_handshake(http::request &req) {
        return req.header_has_token("Connection", "Upgrade", true)
            && req.header_has_token("Upgrade", "websocket", true)
            && req.header_has_value("Sec-WebSocket-Version", "13")
            && req.header_is_nonempty("Sec-WebSocket-Key");
    }

    bool is_handshake(http::response &res) {
        return res.header_has_token("Connection", "Upgrade", true)
            && res.header_has_token("Upgrade", "websocket", true)
            && res.header_has_value("Sec-WebSocket-Version", "13")
            && res.header_is_nonempty("Sec-WebSocket-Accept");
    }
}