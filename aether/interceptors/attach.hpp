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
    std::enable_if_t<std::is_base_of_v<proxy::tcp::intercept::http_interceptor_service::service, T>, 
        proxy::tcp::intercept::interceptor_id>
    attach_http_interceptor(proxy::server &server) {
        auto &&new_service = T();
        return server.interceptors.http.attach(new_service.event(), std::move(new_service));
    }

    template <typename T>
    std::enable_if_t<std::is_base_of_v<proxy::tcp::intercept::http_interceptor_service::functor, T>, 
        proxy::tcp::intercept::interceptor_id>
    attach_http_interceptor(const proxy::tcp::intercept::http_event event, proxy::server &server) {
        return server.interceptors.http.attach(event, T());
    }

    /*
        Attaches the default interceptors.
    */
    void attach_default(proxy::server &server);
}