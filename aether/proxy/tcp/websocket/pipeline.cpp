/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "pipeline.hpp"

namespace proxy::tcp::websocket {
    pipeline::pipeline(const http::exchange &handshake, bool should_intercept) 
        : intercept_flag(should_intercept),
        closed_flag(false),
        closed_by(),
        closed_frame()
    {
        client_key = handshake::get_client_key(handshake.request());
        client_protocol = handshake::get_protocol(handshake.request());
        server_accept = handshake::get_server_accept(handshake.response());
        server_protocol = handshake::get_protocol(handshake.response());
        extensions = handshake::get_extensions(handshake.response());
    }

    bool pipeline::should_intercept() const {
        return intercept_flag;
    }

    void pipeline::set_interception(bool flag) {
        intercept_flag = flag;
    }

    bool pipeline::closed() {
        return closed_flag;
    }

    endpoint pipeline::get_closed_by() const {
        return closed_by;
    }

    close_code pipeline::get_close_code() const {
        return closed_frame.code;
    }

    std::string pipeline::get_close_reason() const {
        return closed_frame.reason;
    }

    close_frame pipeline::get_close_frame() const {
        return closed_frame;
    }

    void pipeline::set_close_state(endpoint closer, const close_frame &frame) {
        std::lock_guard<std::mutex> lock(closing_state_mutex);

        closed_flag = true;
        closed_by = closer;
        closed_frame.code = frame.code;
        closed_frame.reason = frame.reason;
    }

    std::string pipeline::get_client_key() const {
        return client_key;
    }

    std::optional<std::string> pipeline::get_client_protocol() const {
        return client_protocol;
    }

    std::string pipeline::get_server_accept() const {
        return server_accept;
    }

    std::optional<std::string> pipeline::get_server_protocol() const {
        return server_protocol;
    }

    const std::vector<handshake::extension_data> &pipeline::get_extensions() const {
        return extensions;
    }

    bool pipeline::has_message(endpoint caller) {
        std::lock_guard<std::mutex> lock(get_mutex_for_endpoint(caller));
        return !get_message_queue_for_endpoint(caller).empty();
    }

    message &pipeline::next_message(endpoint caller) {
        std::lock_guard<std::mutex> lock(get_mutex_for_endpoint(caller));
        return get_message_queue_for_endpoint(caller).front();
    }

    void pipeline::pop_message(endpoint caller) {
        std::lock_guard<std::mutex> lock(get_mutex_for_endpoint(caller));
        get_message_queue_for_endpoint(caller).pop();
    }

    void pipeline::inject_message(endpoint destination, const message &msg) {
        std::lock_guard<std::mutex> lock(get_mutex_for_endpoint(destination));
        get_message_queue_for_endpoint(destination).push(msg);
    }

    bool pipeline::has_frame(endpoint caller) {
        std::lock_guard<std::mutex> lock(get_mutex_for_endpoint(caller));
        return !get_frame_queue_for_endpoint(caller).empty();
    }

    completed_frame &pipeline::next_frame(endpoint caller) {
        std::lock_guard<std::mutex> lock(get_mutex_for_endpoint(caller));
        return get_frame_queue_for_endpoint(caller).front();
    }

    void pipeline::pop_frame(endpoint caller) {
        std::lock_guard<std::mutex> lock(get_mutex_for_endpoint(caller));
        get_frame_queue_for_endpoint(caller).pop();
    }

    void pipeline::inject_frame(endpoint destination, const completed_frame &frame) {
        // Injecting a close frame is the same as setting the close state
        if (frame.get_type() == opcode::close) {
            set_close_state(~destination, frame.get_close_frame());
        }
        else {
            std::lock_guard<std::mutex> lock(get_mutex_for_endpoint(destination));
            get_frame_queue_for_endpoint(destination).push(frame);
        }
    }
}