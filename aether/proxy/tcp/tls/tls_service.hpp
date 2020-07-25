/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/base_service.hpp>
#include <aether/proxy/tcp/tls/openssl/ssl_context.hpp>
#include <aether/proxy/tcp/tls/handshake/handshake_reader.hpp>
#include <aether/proxy/tcp/tls/handshake/client_hello.hpp>
#include <aether/proxy/tcp/tunnel/tunnel_service.hpp>

namespace proxy::tcp::tls {
    /*
        Service for handling TLS over TCP connections.
        If the incoming client connection does not start with a TLS
            handshake, the data is passed onto the TCP tunnel service.
    */
    class tls_service
        : public base_service {
    private:
        std::unique_ptr<const handshake::client_hello> client_hello_msg;
        handshake::handshake_reader client_hello_reader;

        void read_client_hello();
        void on_read_client_hello(const boost::system::error_code &error, std::size_t bytes_transferred);
        void handle_client_hello();

        void connect_server();
        void on_connect_server(const boost::system::error_code &error);
        void establish_tls_with_server();
        void on_establish_tls_with_server(const boost::system::error_code &error);

        void handle_not_client_hello();

    public:
        tls_service(connection::connection_flow &flow, connection_handler &owner,
            tcp::intercept::interceptor_manager &interceptors);
        void start() override;
    };
}