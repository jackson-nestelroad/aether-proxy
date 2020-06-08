/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <functional>
#include <set>

#include <aether/proxy/tcp/intercept/base_interceptor_service.hpp>
#include <aether/proxy/connection/connection_flow.hpp>
#include <aether/proxy/tcp/http/exchange.hpp>

namespace proxy::tcp::intercept {
    enum class http_events {
        request,
    };

    class http_interceptor_service 
        : public base_interceptor_service<http_events, connection::connection_flow &, http::exchange &> 
    { };
}