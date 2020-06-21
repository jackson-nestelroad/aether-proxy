/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "tls_service.hpp"
#include <aether/proxy/connection_handler.hpp>

namespace proxy::tcp::tls {
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
        // flow.server << client_hello_reader.get_bytes();
        owner.switch_service<tunnel::tunnel_service>();
    }

    void tls_service::handle_client_hello() {
        // TODO: Parse Client Hello data
        out::console::log("TLS!");
        handle_not_client_hello();
    }
}