/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "client_connection.hpp"

namespace proxy::connection {
    client_connection::client_connection(boost::asio::io_context &ioc)
        : base_connection(ioc),
        ssl_method(tcp::tls::openssl::ssl_method::sslv23)
    { }

    void client_connection::establish_tls_async(tcp::tls::openssl::ssl_server_context_args &args, const err_callback &handler) {
        ssl_context = tcp::tls::openssl::create_ssl_context(args.base_args);
        
        if (!SSL_CTX_use_PrivateKey(ssl_context->native_handle(), *args.pkey)) {
            throw error::tls::ssl_context_error_exception { "Failed to set private key" };
        }

        if (!SSL_CTX_use_certificate(ssl_context->native_handle(), *args.cert)) {
            throw error::tls::ssl_context_error_exception { "Failed to set client certificate" };
        }

        if (args.cert_chain.has_value()) {
            const auto &cert_chain = args.cert_chain.value().get();
            if (!cert_chain.empty()) {
                for (const auto &cert : cert_chain) {
                    if (!SSL_CTX_add_extra_chain_cert(ssl_context->native_handle(), *cert)) {
                        throw error::tls::ssl_context_error_exception { "Failed to add certificate to client chain" };
                    }
                }
            }
        }

        if (args.dhparams) {
            if (!SSL_CTX_set_tmp_dh(ssl_context->native_handle(), *args.dhparams)) {
                throw error::tls::ssl_context_error_exception { "Failed to set Diffie-Hellman parameters for client context" };
            }
        }

        secure_socket = std::make_unique<std::remove_reference_t<decltype(*secure_socket)>>(socket, *ssl_context);
        SSL_set_accept_state(secure_socket->native_handle());

        secure_socket->async_handshake(boost::asio::ssl::stream_base::handshake_type::server, input.data_sequence(),
            boost::asio::bind_executor(strand,
                boost::bind(&client_connection::on_handshake, this,
                    boost::asio::placeholders::error, handler)));
    }

    void client_connection::on_handshake(const boost::system::error_code &err, const err_callback &handler) {
        input.reset();

        if (err == boost::system::errc::success) {
            cert = secure_socket->native_handle();

            // Save details about the SSL connection
            sni = SSL_get_servername(secure_socket->native_handle(), SSL_get_servername_type(secure_socket->native_handle()));
            
            const unsigned char *proto = nullptr;
            unsigned int length = 0;
            SSL_get0_alpn_selected(secure_socket->native_handle(), &proto, &length);
            if (proto) {
                alpn = std::string(reinterpret_cast<const char *>(proto), length);
            }

            cipher_name = SSL_get_cipher_name(secure_socket->native_handle());

            std::string version = SSL_get_version(secure_socket->native_handle());
            ssl_method = boost::lexical_cast<tcp::tls::openssl::ssl_method>(version);

            tls_established = true;
        }

        boost::asio::post(ioc, boost::bind(handler, err));
    }
}