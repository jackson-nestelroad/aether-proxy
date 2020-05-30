/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "connection_manager.hpp"

namespace proxy::connection {
    void connection_manager::start(connection_flow::ptr flow) {
        auto new_service = connection_handler::create(flow);
        services.insert(new_service);
        new_service->start(boost::bind(&connection_manager::stop, this, new_service->weak_from_this()));
    }

    void connection_manager::stop(connection_handler::weak_ptr http_service) {
        services.erase(http_service.lock());
    }

    void connection_manager::stop_all() {
        for (auto http_service : services) {
            http_service->stop();
        }
        services.clear();
    }
}