/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/tcp/base_service.hpp>
#include <aether/proxy/tcp/tls/openssl/ssl_context.hpp>
#include <aether/proxy/tcp/tls/x509/client_store.hpp>
#include <aether/proxy/tcp/tls/x509/server_store.hpp>
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
    public:
        static constexpr std::string_view default_alpn = "http/1.1";
        static const std::vector<handshake::cipher_suite_name> default_client_ciphers;

    private:
        static std::unique_ptr<x509::client_store> client_store;
        static std::unique_ptr<x509::server_store> server_store;

        std::unique_ptr<const handshake::client_hello> client_hello_msg;
        handshake::handshake_reader client_hello_reader;
        std::unique_ptr<openssl::ssl_context_args> ssl_client_context_args;
        std::unique_ptr<openssl::ssl_server_context_args> ssl_server_context_args;

        void read_client_hello();
        void on_read_client_hello(const boost::system::error_code &error, std::size_t bytes_transferred);
        void handle_client_hello();

        void connect_server();
        void on_connect_server(const boost::system::error_code &error);
        void establish_tls_with_server();
        void on_establish_tls_with_server(const boost::system::error_code &error);
        
        void establish_tls_with_client();
        tcp::tls::x509::memory_certificate get_certificate_for_client();
        void on_establish_tls_with_client(const boost::system::error_code &error);

        void handle_not_client_hello();

    public:
        tls_service(connection::connection_flow &flow, connection_handler &owner,
            tcp::intercept::interceptor_manager &interceptors);
        void start() override;

        /*
            Create the server's certificate store.
            This must be called after the program::options are read.
        */
        static void create_cert_store();
    };
}