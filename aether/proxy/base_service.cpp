/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "base_service.hpp"
#include "connection_handler.hpp"

namespace proxy {
    base_service::base_service(connection::connection_flow &flow, connection_handler &owner,
        tcp::intercept::interceptor_manager &interceptors)
        : ioc(flow.io_context()),
        flow(flow),
        owner(owner),
        interceptors(interceptors)
    { }

    base_service::~base_service() { }

    void base_service::stop() {
        owner.stop();
    }
}