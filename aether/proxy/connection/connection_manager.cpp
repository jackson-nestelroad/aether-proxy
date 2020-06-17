/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "connection_manager.hpp"

namespace proxy::connection {
    connection_manager::connection_manager(tcp::intercept::interceptor_manager &interceptors)
        : interceptors(interceptors)
    { }

    connection_flow &connection_manager::new_connection(boost::asio::io_service &ios) {
        auto res = connections.emplace(new connection_flow(ios));
        return *res.first->get();
    }
    void connection_manager::start(connection_flow &flow) {
        auto res = services.emplace(new connection_handler(flow, interceptors));
        connection_handler &new_handler = *res.first->get();
        // We use a weak pointer here because the owned object needs to store a reference to itself
        // to be destroyed later
        new_handler.start(boost::bind(&connection_manager::stop, this, new_handler.weak_from_this()));
    }

    void connection_manager::stop(std::weak_ptr<connection_handler> service) {
        // This is the owning object, so no fear in using this weak pointer
        auto shared_service = service.lock();
        connection_flow &flow = shared_service->get_connection_flow();
        services.erase(shared_service);
        connections.erase(flow.shared_from_this());
    }

    void connection_manager::stop_all() {
        for (auto current_service : services) {
            current_service->stop();
        }
        services.clear();
        connections.clear();
    }
}