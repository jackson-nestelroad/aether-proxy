/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "message.hpp"

namespace proxy::tcp::websocket {
    message::message(opcode type, endpoint origin) 
        : type(type),
        origin(origin),
        content(),
        blocked_flag(false)
    { }

    message::message(opcode type, endpoint origin, const std::string &content)
        : type(type),
        origin(origin),
        content(content),
        blocked_flag(false)
    { }

    message::message(endpoint origin, const std::string &content)
        : type(opcode::text),
        origin(origin),
        content(content),
        blocked_flag(false)
    { }

    opcode message::get_type() const {
        return type;
    }

    endpoint message::get_origin() const {
        return origin;
    }

    std::string message::get_content() const {
        return content;
    }

    void message::set_content(const std::string &str) {
        content = str;
    }

    void message::set_content(std::string &&str) {
        content = std::move(str);
    }

    bool message::blocked() const {
        return blocked_flag;
    }

    void message::block() {
        blocked_flag = true;
    }

    std::size_t message::size() const {
        return content.size();
    }
}