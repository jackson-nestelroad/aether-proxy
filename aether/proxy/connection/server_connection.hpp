/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include <aether/proxy/connection/base_connection.hpp>
#include <aether/proxy/types.hpp>

namespace proxy::connection {
    /*
        A connection to the server (wherever the client specifies).
    */
    class server_connection
        : public base_connection {
    private:
        boost::asio::ip::tcp::resolver resolver;
        boost::asio::ip::tcp::endpoint endpoint;
        std::string host;
        port_t port;
        bool is_connected;
        bool is_secure;

        void on_resolve(const boost::system::error_code &err,
            boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
            const err_callback &handler);
        void on_connect(const boost::system::error_code &err,
            boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
            const err_callback &handler);

    public:
        server_connection(boost::asio::io_service &ios);
        void set_host(const std::string &host, port_t port);
        void connect_async(const err_callback &handler);
        bool connected() const;
        bool secure() const;
        std::string get_host() const;
        port_t get_port() const;
        bool is_connected_to(const std::string &host, port_t port) const;
    };
}