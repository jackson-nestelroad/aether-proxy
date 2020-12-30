/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "handshake.hpp"

namespace proxy::tcp::websocket::handshake {
    bool is_handshake(const http::request &req) {
        return req.header_has_token("Connection", "Upgrade", true)
            && req.header_has_token("Upgrade", "websocket", true)
            && req.header_has_value("Sec-WebSocket-Version", "13")
            && req.header_is_nonempty("Sec-WebSocket-Key");
    }

    bool is_handshake(const http::response &res) {
        return res.header_has_token("Connection", "Upgrade", true)
            && res.header_has_token("Upgrade", "websocket", true)
            && res.header_is_nonempty("Sec-WebSocket-Accept");
    }

    std::string get_client_key(const http::message &msg) {
        return msg.get_header("Sec-WebSocket-Key");
    }

    std::string get_server_accept(const http::message &msg) {
        return msg.get_header("Sec-WebSocket-Accept");
    }

    std::optional<std::string> get_protocol(const http::message &msg) {
        return msg.get_optional_header("Sec-WebSocket-Protocol");
    }

    std::vector<extension_data> get_extensions(const http::message &msg) {
        /*
            Multiple extensions may exist across different headers

            Sec-WebSocket-Extensions: mux; max-channels=4; flow-control, deflate-stream
            Sec-WebSocket-Extensions: private-extension

            OR

            Sec-WebSocket-Extensions: deflate-stream, mux; max-channels=4; flow-control, deflate-stream, private-extension
        */
        const auto &extension_headers = msg.get_all_of_header("Sec-WebSocket-Extensions");
        std::vector<extension_data> extensions;
        for (const auto &extension_list : extension_headers) {
            const auto &extension_strings = util::string::split_trim(extension_list, ',');
            for (const auto &ext_string : extension_strings) {
                extensions.push_back(extension_data::from_header_value(ext_string));
            }
        }
        return extensions;
    }
}