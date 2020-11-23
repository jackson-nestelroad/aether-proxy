/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/tcp/intercept/base_interceptor_service.hpp>
#include <aether/proxy/connection/connection_flow.hpp>

namespace proxy::tcp::intercept {
    enum class tunnel_event {
        start,
        stop
    };

    class tunnel_interceptor_service
        : public base_interceptor_service<tunnel_event, connection::connection_flow &>
    { };
}