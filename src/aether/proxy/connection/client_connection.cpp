/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "client_connection.hpp"

#include <boost/asio.hpp>
#include <memory>
#include <string>

#include "aether/proxy/error/exceptions.hpp"
#include "aether/proxy/tls/x509/certificate.hpp"
#include "aether/proxy/types.hpp"

namespace proxy::connection {

client_connection::client_connection(boost::asio::io_context& ioc, server_components& components)
    : base_connection(ioc, components), ssl_method_(tls::openssl::ssl_method::sslv23) {}

void client_connection::establish_tls_async(tls::openssl::ssl_server_context_args& args, err_callback_t handler) {
  ssl_context_ = tls::openssl::create_ssl_context(args.base_args);

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
        if (!SSL_CTX_add_extra_chain_cert(ssl_context_->native_handle(), *cert)) {
          throw error::tls::ssl_context_error_exception{"Failed to add certificate to client chain"};
        }
      }
    }
  }

  if (args.dhpkey) {
    if (!SSL_CTX_set0_tmp_dh_pkey(ssl_context_->native_handle(), *args.dhpkey)) {
      throw error::tls::ssl_context_error_exception{"Failed to set Diffie-Hellman parameters for client context"};
    }
    // The above function "takes ownership" of the dhpkey, but we want to reuse it, so manually increment the reference
    // count.
    args.dhpkey.increment();
  }

  secure_socket_ = std::make_unique<std::remove_reference_t<decltype(*secure_socket_)>>(socket_, *ssl_context_);
  SSL_set_accept_state(secure_socket_->native_handle());

  secure_socket_->async_handshake(
      boost::asio::ssl::stream_base::handshake_type::server, input_.data_sequence(),
      boost::asio::bind_executor(
          strand_, [this, handler = std::move(handler)](const boost::system::error_code& error, std::size_t) mutable {
            on_handshake(std::move(handler), error);
          }));
}

void client_connection::on_handshake(err_callback_t handler, const boost::system::error_code& error) {
  input_.reset();

  if (error == boost::system::errc::success) {
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
    ssl_method_ = boost::lexical_cast<tls::openssl::ssl_method>(version);

    tls_established_ = true;
  } else {
    set_connected(false);
  }

  boost::asio::post(ioc_, [handler = std::move(handler), error]() mutable { handler(error); });
}

}  // namespace proxy::connection
