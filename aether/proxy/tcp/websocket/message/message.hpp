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
        message(opcode type, endpoint origin, const std::string &content);
        message(endpoint origin, const std::string &content);

        opcode get_type() const;
        endpoint get_origin() const;
        std::string get_content() const;
        void set_content(const std::string &str);
        void set_content(std::string &&str);
        bool blocked() const;
        void block();
        std::size_t size() const;
    };
}