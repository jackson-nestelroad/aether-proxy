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

namespace proxy::connection {
    /*
        A thin wrapper for a connection pair (client and server).
        Will always have a client connection, but server connection can be started later.
        Client and server connections should never be instantiated separately to assure they are kept together.
    */
    class connection_flow
        : private boost::noncopyable {
    private:
        connection_flow(io_service::ptr ios);

        io_service::ptr ios;

        // Shared pointers to the two connections to let them use shared_from_this()
        // Private to prevent dangling references

        base_connection::ptr client_ptr;
        base_connection::ptr server_ptr;

    public:
        using ptr = std::shared_ptr<connection_flow>;
        using weak_ptr = std::weak_ptr<connection_flow>;

        // "Interfaces" to the shared pointers

        client_connection &client;
        server_connection &server;

        /*
            Creates a new connection flow, opening up a client socket to receive a request.
        */
        static ptr create(io_service::ptr ios);

        /*
            Sets the server to connect to later.
        */
        void set_server(const std::string &host, port_t port);

        /*
            Connects to a server.
            Set server details using set_server.
        */
        void connect_server_async(const err_callback &handler);
        
        /*
            Disconnects both the client and server connections if applicable.
        */
        void disconnect();

        io_service::ptr io_service() const;
    };
}