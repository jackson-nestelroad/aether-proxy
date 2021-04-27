/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "connection_flow.hpp"

namespace proxy::connection {
    connection_flow::connection_flow(boost::asio::io_context &ioc)
        : ioc(ioc),
        client_ptr(new client_connection(ioc)),
        server_ptr(new server_connection(ioc)),
        client(static_cast<client_connection &>(*client_ptr)),
        server(static_cast<server_connection &>(*server_ptr)),
        target_port(),
        intercept_tls_flag(false),
        intercept_websocket_flag(false)
    { }

    void connection_flow::set_server(std::string_view host, port_t port) {
        if (server.connected()) {
            server.disconnect();
        }
        target_host = host;
        target_port = port;
    }

    void connection_flow::connect_server_async(const err_callback &handler) {
        server.connect_async(target_host, target_port, handler);
    }

    void connection_flow::establish_tls_with_client_async(tcp::tls::openssl::ssl_server_context_args &args, const err_callback &handler) {
        client.establish_tls_async(args, handler);
    }

    void connection_flow::establish_tls_with_server_async(tcp::tls::openssl::ssl_context_args &args, const err_callback &handler) {
        server.establish_tls_async(args, handler);
    }

    void connection_flow::disconnect() {
        client.close();
        server.disconnect();
    }

    boost::asio::io_context &connection_flow::io_context() const {
        return ioc;
    }

    void connection_flow::set_intercept_tls(bool val) {
        intercept_tls_flag = val;
    }

    bool connection_flow::should_intercept_tls() const {
        return intercept_tls_flag;
    }

    void connection_flow::set_intercept_websocket(bool val) {
        intercept_websocket_flag = val;
    }

    bool connection_flow::should_intercept_websocket() const {
        return intercept_websocket_flag;
    }
}