/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "tls_service.hpp"
#include <aether/proxy/connection_handler.hpp>

namespace proxy::tcp::tls {
    std::unique_ptr<x509::client_store> tls_service::client_store;
    std::unique_ptr<x509::server_store> tls_service::server_store;

    tls_service::tls_service(connection::connection_flow &flow, connection_handler &owner,
        tcp::intercept::interceptor_manager &interceptors)
        : base_service(flow, owner, interceptors)
    { }
    
    void tls_service::start() {
        read_client_hello();
    }

    void tls_service::read_client_hello() {
        flow.client.read_async(boost::bind(&tls_service::on_read_client_hello, this,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void tls_service::on_read_client_hello(const boost::system::error_code &error, std::size_t bytes_transferred) {
        if (error != boost::system::errc::success) {
            // There may be data in the stream that is still inteded for the server
            // Switch to a TCP tunnel to be safe
            handle_not_client_hello();
        }
        else {
            try {
                auto buf = flow.client.const_input_buffer();
                std::size_t remaining = client_hello_reader.read(buf, bytes_transferred);

                // Parsing complete
                if (remaining == 0) {
                    handle_client_hello();
                }
                // Need to read again (unlikely)
                else {
                    read_client_hello();
                }
            }
            // The data was not formatted as we would expect, but it still
            // may be intended for the server
            catch (const error::tls::invalid_client_hello_exception &) {
                handle_not_client_hello();
            }
        }
    }

    void tls_service::handle_not_client_hello() {
        // This is NOT a Client Hello message, so TLS is the wrong protocol to use
        // Thus, we forward the data to a TCP tunnel
        owner.switch_service<tunnel::tunnel_service>();
    }

    void tls_service::handle_client_hello() {
        try {
            client_hello_msg = std::make_unique<handshake::client_hello>(
                std::move(handshake::client_hello::from_raw_data(client_hello_reader.get_bytes())));

            connect_server();
        }
        catch (const error::tls::invalid_client_hello_exception &) {
            handle_not_client_hello();
        }
    }

    void tls_service::connect_server() {
        flow.connect_server_async(boost::bind(&tls_service::on_connect_server, this, 
            boost::asio::placeholders::error));
    }

    void tls_service::on_connect_server(const boost::system::error_code &error) {
        if (error != boost::system::errc::success) {
            // TODO: Establish TLS with client to give an error message
            stop();
        }
        else {
            establish_tls_with_server();
        }
    }

    void tls_service::establish_tls_with_server() {
        if (!client_hello_msg) {
            throw error::tls::tls_service_exception { "Must parse Client Hello message before establishing TLS with server" };
        }

        auto context_args = openssl::ssl_context_args::create();

        context_args.verify_file = client_store->cert_file();

        if (client_hello_msg->has_alpn_extension() && !program::options::instance().ssl_negotiate_alpn) {
            // Remove unsupported protocols to be sure the server picks one we can read
            std::copy_if(client_hello_msg->alpn.begin(), client_hello_msg->alpn.end(), std::back_inserter(context_args.alpn_protos),
                [](const std::string &protocol) {
                    return !((protocol.rfind("h2-") == 0) || (protocol == "SPDY"));
                });
            // TODO: Remove this line once HTTP 2 is supported
            context_args.alpn_protos.erase(std::remove(context_args.alpn_protos.begin(), context_args.alpn_protos.end(), "h2"), context_args.alpn_protos.end());
        }

        // TODO: If client TLS is established already, use client's negotiated ALPN by default
        if (flow.client.is_secure()) {

        }

        if (!program::options::instance().ssl_negotiate_ciphers) {
            // Use only ciphers we have named with the server
            std::copy_if(client_hello_msg->cipher_suites.begin(), client_hello_msg->cipher_suites.end(), std::back_inserter(context_args.cipher_suites),
                [](const handshake::cipher_suite_name &cipher) {
                    return handshake::is_valid(cipher);
                });
        }

        flow.establish_tls_with_server_async(context_args, boost::bind(&tls_service::on_establish_tls_with_server, this,
            boost::asio::placeholders::error));
    }

    void tls_service::on_establish_tls_with_server(const boost::system::error_code &error) {
        if (error != boost::system::errc::success) {
            // TODO: Establish TLS with client to give error
            stop();
        }
        else {
            establish_tls_with_client();
        }
    }

    void tls_service::establish_tls_with_client() {
        get_certificate_for_client();
    }

    void tls_service::get_certificate_for_client() {

    }

    void tls_service::on_establish_tls_with_client(const boost::system::error_code &error) {

    }

    void tls_service::create_cert_store() {
        client_store.reset();
        server_store.reset();
        client_store = std::make_unique<x509::client_store>();
        server_store = std::make_unique<x509::server_store>();
    }
}