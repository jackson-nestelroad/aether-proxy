/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "connection_flow.hpp"

namespace proxy::connection {
    connection_flow::connection_flow(boost::asio::io_service &ios)
        : ios(ios),
        client_ptr(new client_connection(ios)),
        server_ptr(new server_connection(ios)),
        client(static_cast<client_connection &>(*client_ptr)),
        server(static_cast<server_connection &>(*server_ptr))
    { }

    void connection_flow::set_server(const std::string &host, port_t port) {
       server.set_host(host, port);
    }

    void connection_flow::connect_server_async(const err_callback &handler) {
        server.connect_async(handler);
    }

    void connection_flow::disconnect() {
        client.close();
        server.close();
    }

    boost::asio::io_service &connection_flow::io_service() const {
        return ios;
    }
}