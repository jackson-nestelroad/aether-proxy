/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "server_connection.hpp"

namespace proxy::connection {
    server_connection::server_connection(boost::asio::io_service &ios)
        : base_connection(ios),
        resolver(ios),
        is_connected(false),
        is_secure(false),
        port()
    { }

    void server_connection::set_host(const std::string &host, port_t port) {
        this->host = host;
        this->port = port;
    }

    void server_connection::connect_async(const err_callback &handler) {
        if (is_connected) {
            close();
            is_connected = false;
        }
        set_timeout();
        boost::asio::ip::tcp::resolver::query query(host, boost::lexical_cast<std::string>(port));
        resolver.async_resolve(query, boost::bind(&server_connection::on_resolve, this,
            boost::asio::placeholders::error, boost::asio::placeholders::iterator, handler));
    }

    void server_connection::on_resolve(const boost::system::error_code &err,
        boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
        const err_callback &handler) {
        timeout.cancel_timeout();
        if (err != boost::system::errc::success) {
            ios.post(boost::bind(handler, err));
        }
        // No endpoints found
        else if (endpoint_iterator == boost::asio::ip::tcp::resolver::iterator()) {
            ios.post(boost::bind(handler, boost::system::errc::make_error_code(boost::system::errc::host_unreachable)));
        }
        else {
            set_timeout();
            endpoint = endpoint_iterator->endpoint();
            auto curr = *endpoint_iterator;
            socket.async_connect(curr, boost::bind(&server_connection::on_connect, this,
                boost::asio::placeholders::error, ++endpoint_iterator, handler));
        }
    }

    void server_connection::on_connect(const boost::system::error_code &err,
        boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
        const err_callback &handler) {
        timeout.cancel_timeout();
        if (err == boost::system::errc::success) {
            is_connected = true;
            if (is_secure) {
                // TODO: Handle SSL handshake
                // Pass a parameter that says to open a secure socket? (SSL)
                ios.post(boost::bind(handler, err));
            }
            else {
                ios.post(boost::bind(handler, err));
            }
        }
        // Didn't connect, but other endpoints to try
        else if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator()) {
            set_timeout();
            endpoint = endpoint_iterator->endpoint();
            auto curr = *endpoint_iterator;
            socket.async_connect(curr, boost::bind(&server_connection::on_connect, this,
                boost::asio::placeholders::error, ++endpoint_iterator, handler));
        }
        // Failed to connect
        else {
            ios.post(boost::bind(handler, err));
        }
    }

    bool server_connection::connected() const {
        return is_connected;
    }

    bool server_connection::secure() const {
        return is_secure;
    }

    std::string server_connection::get_host() const {
        return host;
    }

    port_t server_connection::get_port() const {
        return port;
    }
    
    bool server_connection::is_connected_to(const std::string &host, port_t port) const {
        return is_connected && this->host == host && this->port == port;
    }
}