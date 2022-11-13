/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <array>
#include <boost/system/error_code.hpp>
#include <memory>
#include <string_view>

#include "aether/proxy/base_service.hpp"
#include "aether/proxy/tls/handshake/client_hello.hpp"
#include "aether/proxy/tls/handshake/handshake_reader.hpp"
#include "aether/proxy/tls/openssl/ssl_context.hpp"
#include "aether/proxy/tls/x509/client_store.hpp"
#include "aether/proxy/tls/x509/server_store.hpp"

namespace proxy::tls {
// Service for handling TLS over TCP connections.
// If the incoming client connection does not start with a TLS handshake, the data is passed onto the TCP tunnel
// service.
class tls_service : public base_service {
 public:
  static constexpr std::string_view default_alpn = "http/1.1";

  // See https://ssl-config.mozilla.org/#config=old.
  static constexpr std::array<handshake::cipher_suite_name, 26> default_client_ciphers = {
      handshake::cipher_suite_name::ECDHE_ECDSA_AES128_GCM_SHA256,
      handshake::cipher_suite_name::ECDHE_RSA_AES128_GCM_SHA256,
      handshake::cipher_suite_name::ECDHE_ECDSA_AES256_GCM_SHA384,
      handshake::cipher_suite_name::ECDHE_RSA_AES256_GCM_SHA384,
      handshake::cipher_suite_name::ECDHE_ECDSA_CHACHA20_POLY1305_OLD,
      handshake::cipher_suite_name::ECDHE_RSA_CHACHA20_POLY1305_OLD,
      handshake::cipher_suite_name::DHE_RSA_AES128_GCM_SHA256,
      handshake::cipher_suite_name::DHE_RSA_AES256_GCM_SHA384,
      handshake::cipher_suite_name::DHE_RSA_CHACHA20_POLY1305_OLD,
      handshake::cipher_suite_name::ECDHE_ECDSA_AES128_SHA256,
      handshake::cipher_suite_name::ECDHE_RSA_AES128_SHA256,
      handshake::cipher_suite_name::ECDHE_ECDSA_AES128_SHA,
      handshake::cipher_suite_name::ECDHE_RSA_AES128_SHA,
      handshake::cipher_suite_name::ECDHE_ECDSA_AES256_SHA384,
      handshake::cipher_suite_name::ECDHE_RSA_AES256_SHA384,
      handshake::cipher_suite_name::ECDHE_ECDSA_AES256_SHA,
      handshake::cipher_suite_name::ECDHE_RSA_AES256_SHA,
      handshake::cipher_suite_name::DHE_RSA_AES128_SHA256,
      handshake::cipher_suite_name::DHE_RSA_AES256_SHA256,
      handshake::cipher_suite_name::AES128_GCM_SHA256,
      handshake::cipher_suite_name::AES256_GCM_SHA384,
      handshake::cipher_suite_name::AES128_SHA256,
      handshake::cipher_suite_name::AES256_SHA256,
      handshake::cipher_suite_name::AES128_SHA,
      handshake::cipher_suite_name::AES256_SHA,
      handshake::cipher_suite_name::DES_CBC3_SHA,
  };

  tls_service(connection::connection_flow& flow, connection_handler& owner, server_components& components);

  void start() override;

 private:
  void read_client_hello();
  void on_read_client_hello(const boost::system::error_code& error, std::size_t bytes_transferred);
  void handle_client_hello();

  void connect_server();
  void on_connect_server(const boost::system::error_code& error);
  void establish_tls_with_server();
  void on_establish_tls_with_server(const boost::system::error_code& error);

  void establish_tls_with_client();
  x509::memory_certificate get_certificate_for_client();
  void on_establish_tls_with_client(const boost::system::error_code& error);

  void handle_not_client_hello();

  x509::client_store& client_store_;
  x509::server_store& server_store_;

  std::unique_ptr<const handshake::client_hello> client_hello_msg_;
  handshake::handshake_reader client_hello_reader_;
  std::unique_ptr<openssl::ssl_context_args> ssl_client_context_args_;
  std::unique_ptr<openssl::ssl_server_context_args> ssl_server_context_args_;
};
}  // namespace proxy::tls
