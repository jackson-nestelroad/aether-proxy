/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <optional>

#include <aether/proxy/tcp/http/exchange.hpp>
#include <aether/proxy/tcp/websocket/message/frame.hpp>
#include <aether/proxy/tcp/websocket/handshake/handshake.hpp>
#include <aether/proxy/tcp/websocket/message/message.hpp>
#include <aether/proxy/tcp/websocket/handshake/extension_data.hpp>

namespace proxy::tcp::websocket {
    /*
        Interface for collecting, intercepting, and injecting WebSocket messages
            across a connection flow.
    */
    class pipeline {
    private:
        bool intercept_flag;
        bool closed_flag;

        std::mutex closing_state_mutex;

        endpoint closed_by;
        close_frame closed_frame;

        std::string client_key;
        std::optional<std::string> client_protocol;
        std::string server_accept;
        std::optional<std::string> server_protocol;
        std::vector<handshake::extension_data> extensions;

        // Mutexes and queues used to inject WebSocket messages into the connection flow

        std::mutex client_queue_mutex;
        std::mutex server_queue_mutex;
        std::queue<message> client_message_queue;
        std::queue<message> server_message_queue;
        std::queue<completed_frame> client_frame_queue;
        std::queue<completed_frame> server_frame_queue;

        constexpr std::mutex &get_mutex_for_endpoint(endpoint ep) {
            return ep == endpoint::client ? client_queue_mutex : server_queue_mutex;
        }

        constexpr std::queue<message> &get_message_queue_for_endpoint(endpoint ep) {
            return ep == endpoint::client ? client_message_queue : server_message_queue;
        }

        constexpr const std::queue<message> &get_message_queue_for_endpoint(endpoint ep) const {
            return ep == endpoint::client ? client_message_queue : server_message_queue;
        }

        constexpr std::queue<completed_frame> &get_frame_queue_for_endpoint(endpoint ep) {
            return ep == endpoint::client ? client_frame_queue : server_frame_queue;
        }

        constexpr const std::queue<completed_frame> &get_frame_queue_for_endpoint(endpoint ep) const {
            return ep == endpoint::client ? client_frame_queue : server_frame_queue;
        }

    public:
        pipeline(const http::exchange &handshake, bool should_intercept);
        pipeline() = delete;
        ~pipeline() = default;
        pipeline(const pipeline &other) = delete;
        pipeline &operator=(const pipeline &other) = delete;
        pipeline(pipeline &&other) noexcept = delete;
        pipeline &operator=(pipeline &&other) noexcept = delete;
        
        bool should_intercept() const;
        void set_interception(bool flag);
        bool closed();
        endpoint get_closed_by() const;
        close_code get_close_code() const;
        const std::string &get_close_reason() const;
        close_frame get_close_frame() const;

        /*
            Records how the WebSocket pipeline closed and marks the pipeline as closed.
        */
        void set_close_state(endpoint closer, const close_frame &frame);

        const std::string &get_client_key() const;
        std::optional<std::string_view> get_client_protocol() const;
        const std::string &get_server_accept() const;
        std::optional<std::string_view> get_server_protocol() const;
        const std::vector<handshake::extension_data> &get_extensions() const;

        bool has_message(endpoint caller);
        message &next_message(endpoint caller);
        void pop_message(endpoint caller);
        void inject_message(endpoint destination, const message &msg);
        
        bool has_frame(endpoint caller);
        completed_frame &next_frame(endpoint caller);
        void pop_frame(endpoint caller);
        void inject_frame(endpoint destination, const completed_frame &frame);
    };
}