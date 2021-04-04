/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <variant>

#include <aether/proxy/tcp/websocket/message/close_code.hpp>
#include <aether/proxy/tcp/websocket/message/opcode.hpp>
#include <aether/proxy/tcp/websocket/message/rsv_bits.hpp>
#include <aether/proxy/types.hpp>

namespace proxy::tcp::websocket {

    // frame_header and frame are types that represent raw WebSocket frames as they are parsed

    /*
        Data structure for representing a single WebSocket frame header.
    */
    struct frame_header {
        bool fin;
        rsv_bits rsv;
        opcode type;
        bool mask_bit;
        std::size_t payload_length;
        std::uint32_t mask_key;
    };

    /*
        Data structure representing a single WebSocket frame.
    */
    struct frame
        : public frame_header
    { 
        streambuf content;
    };

    // The following types are abstractions used for processing parsed frames

    /*
        Represents a close WebSocket frame.
    */
    struct close_frame {
        close_code code;
        std::string reason;

        inline close_frame response() const {
            return { code, reason };
        }
    };

    /*
        Represents a pong WebSocket frame.
    */
    struct pong_frame {
        std::string payload;
    };

    /*
        Represents a ping WebSocket frame.
    */
    struct ping_frame {
        std::string payload;

        inline pong_frame response() const {
            return { payload };
        }
    };

    /*
        Represents a message (text or binary) WebSocket frame.
    */
    struct message_frame {
        opcode type;
        bool finished;
        std::string payload;
    };

    /*
        Derived version of std::variant for multiple completed WebSocket frame types.
        Used to store a variety of parsed frames together.
    */
    class completed_frame
        : public std::variant<close_frame, ping_frame, pong_frame, message_frame> {
    public:
        using std::variant<close_frame, ping_frame, pong_frame, message_frame>::variant;

        constexpr const close_frame &get_close_frame() const {
            return std::get<close_frame>(*this);
        }

        constexpr close_frame &get_close_frame() {
            return std::get<close_frame>(*this);
        }

        constexpr const ping_frame &get_ping_frame() const {
            return std::get<ping_frame>(*this);
        }

        constexpr ping_frame &get_ping_frame() {
            return std::get<ping_frame>(*this);
        }

        constexpr const pong_frame &get_pong_frame() const {
            return std::get<pong_frame>(*this);
        }

        constexpr pong_frame &get_pong_frame() {
            return std::get<pong_frame>(*this);
        }

        constexpr const message_frame &get_message_frame() const {
            return std::get<message_frame>(*this);
        }

        constexpr message_frame &get_message_frame() {
            return std::get<message_frame>(*this);
        }

        constexpr opcode get_type() const {
            if (std::holds_alternative<close_frame>(*this)) {
                return opcode::close;
            }
            if (std::holds_alternative<ping_frame>(*this)) {
                return opcode::ping;
            }
            if (std::holds_alternative<pong_frame>(*this)) {
                return opcode::pong;
            }
            if (std::holds_alternative<message_frame>(*this)) {
                return get_message_frame().type;
            }
            return opcode::max;
        }
    };
}