/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "tunnel_service.hpp"

namespace proxy::tcp::tunnel {
    tunnel_service::tunnel_service(connection::connection_flow::ptr flow, connection_handler &owner)
        : base_service(flow, owner),
        upstream(static_cast<connection::base_connection &>(flow->client), static_cast<connection::base_connection &>(flow->server)),
        downstream(static_cast<connection::base_connection &>(flow->server), static_cast<connection::base_connection &>(flow->client))
    { }

    void tunnel_service::start() {
        if (!flow->server.connected()) {
            connect_server();
        }
        else {
            initiate_tunnel();
        }
    }

    void tunnel_service::connect_server() {
        flow->connect_server_async(boost::bind(&tunnel_service::on_connect_server, this,
            boost::asio::placeholders::error));
    }

    void tunnel_service::on_connect_server(const boost::system::error_code &error) {
        if (error != boost::system::errc::success) {
            stop();
        }
        else {
            initiate_tunnel();
        }
    }

    void tunnel_service::initiate_tunnel() {
        finished.store(0);
        downstream.start(boost::bind(&tunnel_service::on_finish, this));
        upstream.start(boost::bind(&tunnel_service::on_finish, this));
    }

    void tunnel_service::on_finish() {
        if (downstream.finished() && upstream.finished()) {
            stop();
        }
    }
}