/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "base_service.hpp"
#include <aether/proxy/connection_handler.hpp>

namespace proxy::tcp {
    base_service::base_service(connection::connection_flow &flow, connection_handler &owner,
        tcp::intercept::interceptor_manager &interceptors)
        : ioc(flow.io_context()),
        flow(flow),
        owner(owner),
        interceptors(interceptors)
    { }

    base_service::~base_service() { }

    void base_service::set_server(std::string_view host, port_t port) {
        if (!flow.server.is_connected_to(host, port)) {
            flow.set_server(host, port);
            if (flow.server.connected()) {
                interceptors.server.run(intercept::server_event::disconnect, flow);
            }
            bool is_forbidden = std::find(forbidden_hosts.begin(), forbidden_hosts.end(), host) == forbidden_hosts.end()
                && port == program::options::instance().port;
            if (is_forbidden) {
                throw error::self_connect_exception { };
            }
        }
    }

    void base_service::connect_server_async(const err_callback &handler) {
        flow.connect_server_async(boost::bind(&base_service::on_connect_server, this,
            boost::asio::placeholders::error, handler));
    }

    void base_service::on_connect_server(const boost::system::error_code &error, const err_callback &handler) {
        if (error == boost::system::errc::success) {
            interceptors.server.run(intercept::server_event::connect, flow);
        }
        handler(error);
    }

    void base_service::stop() {
        owner.stop();
    }
}