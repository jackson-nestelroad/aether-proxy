/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "connection_handler.hpp"

namespace proxy {
    connection_handler::connection_handler(connection::connection_flow::ptr flow)
        : ios(flow->io_service()), 
        flow(flow)
    { }

    connection_handler::ptr connection_handler::create(connection::connection_flow::ptr flow) {
        return ptr(new connection_handler(flow));
    }

    void connection_handler::start(const callback &handler) {
        on_finished = handler;
        http_service.reset(new tcp::http::http1::http_service(flow, *this));
        http_service->start();
    }

    void connection_handler::stop() {
        http_service.reset();
        flow->disconnect();
        on_finished();
    }
}