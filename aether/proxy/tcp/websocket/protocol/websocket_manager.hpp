/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>
#include <vector>

#include <aether/proxy/tcp/websocket/handshake/extension_data.hpp>
#include <aether/proxy/tcp/websocket/protocol/frame_parser.hpp>

namespace proxy::tcp::websocket::protocol {
    /*
        Class for managing WebSocket data parsing and serialization.
    */
    class websocket_manager {
    private:
        endpoint ep;
        std::vector<std::unique_ptr<extensions::extension>> extensions;
        frame_parser fparser;

        void process_close_frame(frame &in, close_frame &out);

    public:
        websocket_manager(endpoint ep, const std::vector<handshake::extension_data> &extension_data);
        websocket_manager() = delete;
        ~websocket_manager() = default;
        websocket_manager(const websocket_manager &other) = delete;
        websocket_manager &operator=(const websocket_manager &other) = delete;
        websocket_manager(websocket_manager &&other) noexcept = delete;
        websocket_manager &operator=(websocket_manager &&other) noexcept = delete;

        /*
            Parses the data in the input stream, returning out any completed frames.
            This method exits in one of three ways:
                1. Normally with no return code.
                2. Close code given that must be used to close the WebSocket connection.
                3. Proxy exception that must be caught. The WebSocket connection must be closed,
                    but any close code may be used that the caller sees fit.
        */
        std::vector<completed_frame> parse(streambuf &input, std::optional<close_code> &should_close);

        /*
            Serializes a frame interface, placing it in the output buffer.
        */
        void serialize(streambuf &output, completed_frame &&frame);
    };
}