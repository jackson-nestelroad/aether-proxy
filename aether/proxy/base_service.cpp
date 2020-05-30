/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "base_service.hpp"
#include "connection_handler.hpp"

namespace proxy {
    base_service::base_service(connection::connection_flow::ptr flow, connection_handler &owner)
        : ios(flow->io_service()),
        flow(flow),
        owner(owner)
    { }

    base_service::~base_service() { }

    void base_service::stop() {
        owner.stop();
    }
}