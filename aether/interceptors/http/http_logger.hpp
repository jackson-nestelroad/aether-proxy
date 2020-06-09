/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <queue>
#include <thread>

#include <aether/proxy/server.hpp>
#include <aether/interceptors/common/logging_mutex.hpp>

namespace interceptors::http {
    /*
        Logging service for all HTTP/1.1 requests.
    */
    template <std::ostream *strm>
    class logging_service
        : proxy::tcp::intercept::http_interceptor_service::service,
        common::logging_mutex<strm> 
    {
    public:
        void operator()(proxy::connection::connection_flow &flow, proxy::tcp::http::exchange &exch) override {
            this->lock();
            *strm << exch.get_request().absolute_request_line_string() << std::endl;
            strm->flush();
            this->unlock();
        }

        proxy::tcp::intercept::http_event event() const override {
            return proxy::tcp::intercept::http_event::any;
        }
    };
}