/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include <aether/proxy/connection/base_connection.hpp>
#include <aether/proxy/tcp/tls/x509/certificate.hpp>
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
        bool is_connected;

        std::string host;
        port_t port;

        std::vector<tcp::tls::x509::certificate> cert_chain;

        void on_resolve(const boost::system::error_code &err,
            boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
            const err_callback &handler);
        void on_connect(const boost::system::error_code &err,
            boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
            const err_callback &handler);
        void on_handshake(const boost::system::error_code &err, const err_callback &handler);

    public:
        server_connection(boost::asio::io_service &ios);
        void connect_async(const std::string &host, port_t port, const err_callback &handler);
        void establish_tls_async(tcp::tls::openssl::ssl_context_args &args, const err_callback &handler);

        bool connected() const;
        std::string get_host() const;
        port_t get_port() const;
        bool is_connected_to(const std::string &host, port_t port) const;
        std::vector<tcp::tls::x509::certificate> get_cert_chain() const;
    };
}