/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "tunnel_service.hpp"

namespace proxy::tcp::tunnel {
    tunnel_service::tunnel_service(connection::connection_flow &flow, connection_handler &owner,
        tcp::intercept::interceptor_manager &interceptors)
        : base_service(flow, owner, interceptors),
        upstream(static_cast<connection::base_connection &>(flow.client), static_cast<connection::base_connection &>(flow.server)),
        downstream(static_cast<connection::base_connection &>(flow.server), static_cast<connection::base_connection &>(flow.client))
    { }

    void tunnel_service::start() {
        if (!flow.server.connected()) {
            connect_server();
        }
        else {
            initiate_tunnel();
        }
    }

    void tunnel_service::connect_server() {
        connect_server_async(boost::bind(&tunnel_service::on_connect_server, this,
            boost::asio::placeholders::error));
    }

    void tunnel_service::on_connect_server(const boost::system::error_code &error) {
        if (error != boost::system::errc::success) {
            flow.error.set_boost_error(error);
            stop();
        }
        else {
            initiate_tunnel();
        }
    }

    void tunnel_service::initiate_tunnel() {
        interceptors.tunnel.run(intercept::tunnel_event::start, flow);
        downstream.start(boost::bind(&tunnel_service::on_finish, this));
        upstream.start(boost::bind(&tunnel_service::on_finish, this));
    }

    void tunnel_service::on_finish() {
        if (downstream.finished() && upstream.finished()) {
            interceptors.tunnel.run(intercept::tunnel_event::stop, flow);
            stop();
        }
    }
}