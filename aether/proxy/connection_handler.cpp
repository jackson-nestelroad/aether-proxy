/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "connection_handler.hpp"

namespace proxy {
    connection_handler::connection_handler(connection::connection_flow &flow, 
        tcp::intercept::interceptor_manager &interceptors)
        : ios(flow.io_service()), 
        flow(flow),
        interceptors(interceptors)
    { }

    void connection_handler::start(const callback &handler) {
        on_finished = handler;
        current_service.reset(new tcp::http::http1::http_service(flow, *this, interceptors));
        current_service->start();
    }

    void connection_handler::stop() {
        current_service.reset();
        flow.disconnect();
        on_finished();
    }

    connection::connection_flow &connection_handler::get_connection_flow() const {
        return flow;
    }
}