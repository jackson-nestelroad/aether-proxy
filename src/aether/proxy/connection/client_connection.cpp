/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "client_connection.hpp"

#include <boost/asio.hpp>
#include <memory>
#include <string>

#include "aether/proxy/error/exceptions.hpp"
#include "aether/proxy/tcp/tls/x509/certificate.hpp"
#include "aether/proxy/types.hpp"

namespace proxy::connection {
client_connection::client_connection(boost::asio::io_context& ioc, server_components& components)
    : base_connection(ioc, components), ssl_method_(tcp::tls::openssl::ssl_method::sslv23) {}

void client_connection::establish_tls_async(tcp::tls::openssl::ssl_server_context_args& args,
                                            const err_callback_t& handler) {
  ssl_context_ = tcp::tls::openssl::create_ssl_context(args.base_args);

  if (!SSL_CTX_use_PrivateKey(ssl_context_->native_handle(), *args.pkey)) {
    throw error::tls::ssl_context_error_exception{"Failed to set private key"};
  }

  if (!SSL_CTX_use_certificate(ssl_context_->native_handle(), *args.cert)) {
    throw error::tls::ssl_context_error_exception{"Failed to set client certificate"};
  }

  if (args.cert_chain.has_value()) {
    const auto& cert_chain = args.cert_chain.value().get();
    if (!cert_chain.empty()) {
      for (const auto& cert : cert_chain) {
        if (!SSL_CTX_add_extra_chain_cert(ssl_context_->native_handle(), *cert_)) {
          throw error::tls::ssl_context_error_exception{"Failed to add certificate to client chain"};
        }
      }
    }
  }

  if (args.dhparams) {
    if (!SSL_CTX_set_tmp_dh(ssl_context_->native_handle(), *args.dhparams)) {
      throw error::tls::ssl_context_error_exception{"Failed to set Diffie-Hellman parameters for client context"};
    }
  }

  secure_socket_ = std::make_unique<std::remove_reference_t<decltype(*secure_socket_)>>(socket_, *ssl_context_);
  SSL_set_accept_state(secure_socket_->native_handle());

  secure_socket_->async_handshake(
      boost::asio::ssl::stream_base::handshake_type::server, input_.data_sequence(),
      boost::asio::bind_executor(
          strand_, boost::bind(&client_connection::on_handshake, this, boost::asio::placeholders::error, handler)));
}

void client_connection::on_handshake(const boost::system::error_code& err, const err_callback_t& handler) {
  input_.reset();

  if (err == boost::system::errc::success) {
    cert_ = secure_socket_->native_handle();

    // Save details about the SSL connection.
    sni_ =
        SSL_get_servername(secure_socket_->native_handle(), SSL_get_servername_type(secure_socket_->native_handle()));

    const unsigned char* proto = nullptr;
    unsigned int length = 0;
    SSL_get0_alpn_selected(secure_socket_->native_handle(), &proto, &length);
    if (proto) {
      alpn_ = std::string(reinterpret_cast<const char*>(proto), length);
    }

    cipher_name_ = SSL_get_cipher_name(secure_socket_->native_handle());

    // TODO: string_view here and enable lexical_cast on string_view.
    std::string version = SSL_get_version(secure_socket_->native_handle());
    ssl_method_ = boost::lexical_cast<tcp::tls::openssl::ssl_method>(version);

    tls_established_ = true;
  }

  boost::asio::post(ioc_, boost::bind(handler, err));
}

}  // namespace proxy::connection