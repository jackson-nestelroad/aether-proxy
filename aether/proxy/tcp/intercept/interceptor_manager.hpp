/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/tcp/intercept/http_interceptor_service.hpp>

namespace proxy::tcp::intercept {
    /*
        Class for holding all interceptor service instances.
    */
    class interceptor_manager
        : private boost::noncopyable {
    public:
        http_interceptor_service http;
    };
}