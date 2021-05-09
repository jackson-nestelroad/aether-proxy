/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>

#include <aether/proxy/tcp/websocket/message/endpoint.hpp>
#include <aether/proxy/tcp/websocket/message/opcode.hpp>

namespace proxy::tcp::websocket {
    /*
        Represents a finished WebSocket message, which is a collection of frames.
    */
    class message {
    private:
        opcode type;
        endpoint origin;
        std::string content;
        bool blocked_flag;

    public:
        message(opcode type, endpoint origin);
        message(opcode type, endpoint origin, std::string_view content);
        message(endpoint origin, std::string_view content);
        message() = delete;
        ~message() = default;
        message(const message &other) = default;
        message &operator=(const message &other) = default;
        message(message &&other) noexcept = default;
        message &operator=(message &&other) noexcept = default;

        opcode get_type() const;
        endpoint get_origin() const;
        const std::string &get_content() const;
        void set_content(std::string_view str);
        void set_content(std::string &&str);
        bool blocked() const;
        void block();
        std::size_t size() const;
    };
}