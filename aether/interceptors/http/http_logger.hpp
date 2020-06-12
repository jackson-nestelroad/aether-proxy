/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <queue>
#include <thread>

#include <aether/proxy/server.hpp>
#include <aether/util/console.hpp>

namespace interceptors::http {
    /*
        Logging service for all HTTP/1.1 requests.
    */
    template <std::ostream *strm>
    class http_logger
        : proxy::tcp::intercept::http_interceptor_service::service
    {
    public:
        void operator()(proxy::connection::connection_flow &flow, proxy::tcp::http::exchange &exch) override {
            out::logging_mutex<strm>::lock();
            *strm << exch.get_request().absolute_request_line_string() << std::endl;
            strm->flush();
            out::logging_mutex<strm>::unlock();
        }

        proxy::tcp::intercept::http_event event() const override {
            return proxy::tcp::intercept::http_event::any;
        }
    };
}