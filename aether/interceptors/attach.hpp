/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/server.hpp>

// Functions for attaching interceptors to a server
// Small abstraction of server.interceptors.http.attach.

namespace interceptors {
    template <typename T>
    std::enable_if_t<std::is_base_of_v<proxy::tcp::intercept::http_interceptor_service::service, T>, void> 
    attach_http_interceptor(proxy::server &server) {
        auto &&new_service = T();
        server.interceptors.http.attach(new_service.event(), std::move(new_service));
    }

    template <typename T>
    std::enable_if_t<std::is_base_of_v<proxy::tcp::intercept::http_interceptor_service::functor, T>, void>
    attach_http_interceptor(const proxy::tcp::intercept::http_event event, proxy::server &server) {
        server.interceptors.http.attach(event, T());
    }

    /*
        Attaches the interceptors corresponding to the program's command-line options.
    */
    void attach_options(proxy::server &server, program::options &options);
}