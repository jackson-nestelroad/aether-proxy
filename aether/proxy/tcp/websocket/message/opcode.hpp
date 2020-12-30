/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>
#include <boost/lexical_cast.hpp>

#include <aether/proxy/error/exceptions.hpp>

#define WEBSOCKET_OPCODES(X) \
X(continuation, 0x0, "continue") \
X(text, 0x1, "text") \
X(binary, 0x2, "binary") \
X(close, 0x8, "close") \
X(ping, 0x9, "ping") \
X(pong, 0xA, "pong") \
X(max, 0xF, "max")

namespace proxy::tcp::websocket {
    /*
        Enumeration type for WebSocket opcodes.
    */
    enum class opcode {
#define X(name, num, string) name = num,
        WEBSOCKET_OPCODES(X)
#undef X
    };

    constexpr bool is_control(opcode type) {
        return static_cast<std::uint8_t>(type) & 0x8;
    }

    namespace convert {
        /*
            Converts a WebSocket opcode to string.
        */
        std::string_view to_string(opcode o);
    }

    std::ostream &operator<<(std::ostream &output, opcode o);
}