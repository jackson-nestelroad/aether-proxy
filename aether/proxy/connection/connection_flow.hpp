/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <optional>
#include <boost/asio.hpp>

#include <aether/proxy/connection/client_connection.hpp>
#include <aether/proxy/connection/server_connection.hpp>
#include <aether/proxy/error/exceptions.hpp>
#include <aether/proxy/error/error_state.hpp>
#include <aether/util/identifiable.hpp>

namespace proxy::connection {
    /*
        A thin wrapper for a connection pair (client and server).
        Will always have a client connection, but server connection can be started later.
        Client and server connections should never be instantiated separately to assure they are kept together.
    */
    class connection_flow
        : public util::id::identifiable<std::size_t>
    {
    private:
        boost::asio::io_context &ioc;

        // Shared pointers to the two connections to let them use shared_from_this()
        // Private to prevent dangling references

        base_connection::ptr client_ptr;
        base_connection::ptr server_ptr;

        std::string target_host;
        port_t target_port;

        bool intercept_tls_flag;
        bool intercept_websocket_flag;

    public:
        // "Interfaces" to the shared pointers

        client_connection &client;
        server_connection &server;

        error::error_state error;

        connection_flow(boost::asio::io_context &ioc, server_components &components);
        connection_flow() = delete;
        ~connection_flow() = default;
        connection_flow(const connection_flow &other) = delete;
        connection_flow &operator=(const connection_flow &other) = delete;
        connection_flow(connection_flow &&other) noexcept = delete;
        connection_flow &operator=(connection_flow &&other) noexcept = delete;

        /*
            Sets the server to connect to later.
            Any existing server connection is closed.
        */
        void set_server(std::string_view host, port_t port);

        /*
            Connects to a server.
            Set server details using set_server.
        */
        void connect_server_async(const err_callback &handler);

        /*
            Establishes a TLS connection with the client.
        */
        void establish_tls_with_client_async(tcp::tls::openssl::ssl_server_context_args &args, const err_callback &handler);

        /*
            Establishes a TLS connection with the server.
            Set server details using set_server.
        */
        void establish_tls_with_server_async(tcp::tls::openssl::ssl_context_args &args, const err_callback &handler);
        
        /*
            Disconnects both the client and server connections if applicable.
        */
        void disconnect();

        /*
            Marks the connection flow for TLS interception in passthrough mode.
        */
        void set_intercept_tls(bool val);

        /*
            Returns if the connection flow should be intercepted using TLS.
        */
        bool should_intercept_tls() const;

        /*
            Marks the connection flow for WebSocket interception in passthrough mode.
        */
        void set_intercept_websocket(bool val);

        /*
            Returns if the connection flow's WebSocket messages should be intercepted.
        */
        bool should_intercept_websocket() const;

        boost::asio::io_context &io_context() const;
    };
}