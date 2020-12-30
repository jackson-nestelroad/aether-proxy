/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/tcp/intercept/base_interceptor_service.hpp>
#include <aether/proxy/connection/connection_flow.hpp>
#include <aether/proxy/tcp/websocket/pipeline.hpp>

namespace proxy::tcp::intercept {
    enum class websocket_event {
        start,
        stop,
        error
    };

    enum class websocket_message_event {
        received
    };

    class websocket_interceptor_service
        : public base_interceptor_service<websocket_event, connection::connection_flow &, websocket::pipeline &>
    { };

    class websocket_message_interceptor_service
        : public base_interceptor_service<websocket_message_event, connection::connection_flow &, websocket::pipeline &, websocket::message &>
    { };
}