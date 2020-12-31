/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <queue>
#include <thread>

#include <aether/proxy/server.hpp>
#include <aether/util/console.hpp>

namespace interceptors {
    // TODO: SFINAE here
    /*
        Logging service for all HTTP/1.1 requests.
    */
    template <typename LogStream>
    class http_logger
        : proxy::tcp::intercept::http_interceptor_service::service
    {
    public:
        void operator()(proxy::connection::connection_flow &flow, proxy::tcp::http::exchange &exch) override {
            LogStream::log(exch.request().absolute_request_line_string());
        }
        proxy::tcp::intercept::http_event event() const override {
            return proxy::tcp::intercept::http_event::any_request;
        }
    };
}