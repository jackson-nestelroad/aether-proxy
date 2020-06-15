/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/base_service.hpp>

namespace proxy::tcp::tls {
    /*
        Service for handling TLS over TCP connections.
        If the incoming client connection does not start with a TLS
            handshake, the data is passed onto the TCP tunnel service.
    */
    class tls_service
        : public base_service {
    private:

    public:
        tls_service(connection::connection_flow &flow, connection_handler &owner,
            tcp::intercept::interceptor_manager &interceptors);
        void start() override;
    };
}