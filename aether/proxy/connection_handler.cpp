/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "connection_handler.hpp"
#include <aether/proxy/server_components.hpp>

namespace proxy {
    connection_handler::connection_handler(connection::connection_flow &flow, server_components &components)
        : ioc(flow.io_context()),
        flow(flow),
        components(components)
    { }

    void connection_handler::start(const callback &handler) {
        on_finished = handler;
        current_service = std::make_unique<tcp::http::http1::http_service>(flow, *this, components);
        current_service->start();
    }

    void connection_handler::stop() {
        flow.disconnect();
        current_service.reset();
        on_finished();
    }

    connection::connection_flow &connection_handler::get_connection_flow() const {
        return flow;
    }
}