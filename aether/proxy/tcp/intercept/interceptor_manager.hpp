/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/tcp/intercept/server_interceptor_service.hpp>
#include <aether/proxy/tcp/intercept/http_interceptor_service.hpp>
#include <aether/proxy/tcp/intercept/tunnel_interceptor_service.hpp>
#include <aether/proxy/tcp/intercept/tls_interceptor_service.hpp>

namespace proxy::tcp::intercept {
    /*
        Class for holding all interceptor service instances.
    */
    class interceptor_manager
        : private boost::noncopyable {
    public:
        server_interceptor_service server;
        http_interceptor_service http;
        tunnel_interceptor_service tunnel;
        tls_interceptor_service tls;
    };
}