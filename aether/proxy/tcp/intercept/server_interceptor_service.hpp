/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/tcp/intercept/base_interceptor_service.hpp>
#include <aether/proxy/connection/server_connection.hpp>

namespace proxy::tcp::intercept {
    enum class server_event {
        connect,
        disconnect
    };

    class server_interceptor_service
        : public base_interceptor_service<server_event, connection::server_connection &>
    { };
}