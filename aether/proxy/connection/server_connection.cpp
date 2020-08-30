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
        port(),
        cert(nullptr)
    { }

    void server_connection::connect_async(const std::string &host, port_t port, const err_callback &handler) {
        // Already have an open connection
        if (is_connected_to(host, port) && !has_been_closed()) {
            ios.post(boost::bind(handler, boost::system::errc::make_error_code(boost::system::errc::success)));
            return;
        }
        // Need a new connection
        else if (is_connected) {
            close();
            is_connected = false;
        }

        this->host = host;
        this->port = port;

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
            auto &curr = *endpoint_iterator;
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
            ios.post(boost::bind(handler, err));
        }
        // Didn't connect, but other endpoints to try
        else if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator()) {
            set_timeout();
            endpoint = endpoint_iterator->endpoint();
            auto &curr = *endpoint_iterator;
            socket.async_connect(curr, boost::bind(&server_connection::on_connect, this,
                boost::asio::placeholders::error, ++endpoint_iterator, handler));
        }
        // Failed to connect
        else {
            ios.post(boost::bind(handler, err));
        }
    }

    void server_connection::establish_tls_async(const tcp::tls::openssl::ssl_context_args &args, const err_callback &handler) {
        ssl_context = tcp::tls::openssl::create_client_context(args);
        secure_socket = std::make_unique<std::remove_reference_t<decltype(*secure_socket)>>(socket, *ssl_context);
        
        SSL_set_connect_state(secure_socket->native_handle());

        if (!SSL_set_tlsext_host_name(secure_socket->native_handle(), host.c_str())) {
            throw error::tls::ssl_context_exception { "Failed to set SNI extension" };
        }

        tcp::tls::openssl::enable_hostname_verification(*ssl_context, host);

        secure_socket->async_handshake(boost::asio::ssl::stream_base::handshake_type::client, boost::bind(&server_connection::on_handshake, this,
            boost::asio::placeholders::error, handler));
    }

    void server_connection::on_handshake(const boost::system::error_code &error, const err_callback &handler) {
        if (error == boost::system::errc::success) {
            // Get server's certificate
            cert = secure_socket->native_handle();

            // Get certificate chain
            STACK_OF(X509) *chain = SSL_get_peer_cert_chain(secure_socket->native_handle());
            int len = sk_X509_num(chain);
            for (int i = 0; i < len; ++i) {
                X509 *cert = sk_X509_value(chain, i);
                cert_chain.emplace_back(cert);
            }

            tls_established = true;
        }

        ios.post(boost::bind(handler, error));
    }

    bool server_connection::connected() const {
        return is_connected;
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