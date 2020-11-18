/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>

#include <aether/proxy/connection/base_connection.hpp>
#include <aether/proxy/tcp/tls/x509/certificate.hpp>

namespace proxy::connection {
    /*
        A connection to the client (whoever initiated the request).
    */
    class client_connection
        : public base_connection {
    private:
        std::string sni;
        std::string cipher_name;
        tcp::tls::openssl::ssl_method ssl_method;

        void on_handshake(const boost::system::error_code &err, const err_callback &handler);

    public:
        client_connection(boost::asio::io_context &ioc);
        void establish_tls_async(tcp::tls::openssl::ssl_server_context_args &args, const err_callback &handler);
    };
}