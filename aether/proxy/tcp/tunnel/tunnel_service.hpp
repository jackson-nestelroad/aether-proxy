/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/tcp/base_service.hpp>
#include <aether/proxy/tcp/tunnel/tunnel_loop.hpp>

namespace proxy::tcp::tunnel {
    /*
        Service for a TCP tunnel between two sockets.
        No interception is possible here.
        Currently used for HTTPS/TLS and WebSockets.
    */
    class tunnel_service
        : public base_service {
    private:
        tunnel_loop upstream;
        tunnel_loop downstream;

        void connect_server();
        void on_connect_server(const boost::system::error_code &error);
        void initiate_tunnel();
        void on_finish();

    public:
        tunnel_service(connection::connection_flow &flow, connection_handler &owner,
            tcp::intercept::interceptor_manager &interceptors);
        void start() override;
    };
}