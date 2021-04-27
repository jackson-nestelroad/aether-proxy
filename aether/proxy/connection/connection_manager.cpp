/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "connection_manager.hpp"

namespace proxy::connection {
    connection_manager::connection_manager(tcp::intercept::interceptor_manager &interceptors)
        : interceptors(interceptors)
    { }

    connection_flow &connection_manager::new_connection(boost::asio::io_context &ioc) {
        auto ptr = std::make_unique<connection_flow>(ioc);
        std::lock_guard<std::mutex> lock(data_mutex);
        auto res = connections.emplace(ptr->id(), std::move(ptr));
        return *res.first->second;
    }
    
    void connection_manager::start_service(connection_flow &flow) {
        auto res = services.emplace(std::make_unique<connection_handler>(flow, interceptors));
        connection_handler &new_handler = *res.first->get();
        new_handler.start(boost::bind(&connection_manager::stop, this, std::cref(*res.first)));
    }

    void connection_manager::start(connection_flow &flow) {
        std::lock_guard<std::mutex> lock(data_mutex);

        // TODO: Some way to control how many connections we are servicing at a time
        start_service(flow);
    }

    void connection_manager::destroy(connection_flow &flow) {
        std::lock_guard<std::mutex> lock(data_mutex);
        connections.erase(flow.id());
    }

    void connection_manager::stop(const std::unique_ptr<connection_handler> &service_ptr) {
        connection_flow &flow = service_ptr->get_connection_flow();
        std::lock_guard<std::mutex> lock(data_mutex);
        services.erase(service_ptr);
        connections.erase(flow.id());
    }

    void connection_manager::stop_all() {
        std::lock_guard<std::mutex> lock(data_mutex);

        for (auto &current_service : services) {
            current_service->stop();
        }

        services.clear();
        connections.clear();
    }

    std::size_t connection_manager::total_connection_count() const {
        return connections.size();
    }

    std::size_t connection_manager::active_connection_count() const {
        return services.size();
    }

    std::size_t connection_manager::pending_connection_count() const {
        return 0;
    }
}