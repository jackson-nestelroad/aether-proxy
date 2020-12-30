/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/tcp/intercept/base_interceptor_service.hpp>
#include <aether/proxy/connection/connection_flow.hpp>
#include <aether/proxy/tcp/http/exchange.hpp>

namespace proxy::tcp::intercept {
    enum class http_event {
        request,
        connect,
        any_request,
        websocket_handshake,
        response,
        error,
    };

    class http_interceptor_service 
        : public base_interceptor_service<http_event, connection::connection_flow &, http::exchange &> 
    { };
}