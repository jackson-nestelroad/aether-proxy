/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "tls_service.hpp"

#include <algorithm>
#include <boost/system/error_code.hpp>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "aether/proxy/base_service.hpp"
#include "aether/proxy/connection_handler.hpp"
#include "aether/proxy/error/error.hpp"
#include "aether/proxy/error/exceptions.hpp"
#include "aether/proxy/http/http1/http_service.hpp"
#include "aether/proxy/server_components.hpp"
#include "aether/proxy/tls/handshake/client_hello.hpp"
#include "aether/proxy/tls/handshake/handshake_reader.hpp"
#include "aether/proxy/tls/openssl/ssl_context.hpp"
#include "aether/proxy/tls/x509/client_store.hpp"
#include "aether/proxy/tls/x509/server_store.hpp"
#include "aether/proxy/tunnel/tunnel_service.hpp"
#include "aether/util/console.hpp"
#include "aether/util/result_macros.hpp"

namespace proxy::tls {

tls_service::tls_service(connection::connection_flow& flow, connection_handler& owner, server_components& components)
    : base_service(flow, owner, components),
      client_store_(components.client_store()),
      server_store_(components.server_store()) {}

void tls_service::start() { read_client_hello(); }

void tls_service::read_client_hello() {
  flow_.client.read_async(std::bind_front(&tls_service::on_read_client_hello, this));
}

void tls_service::on_read_client_hello(const boost::system::error_code& error, std::size_t bytes_transferred) {
  if (error != boost::system::errc::success) {
    // There may be data in the stream that is still intended for the server.
    // Switch to a TCP tunnel to be safe.
    handle_not_client_hello();
  } else {
    auto buf = flow_.client.const_input_buffer();
    result<std::size_t> remaining = client_hello_reader_.read(buf, bytes_transferred);
    if (!remaining.is_ok()) {
      // The data was not formatted as we would expect, but it still may be intended for the server.
      handle_not_client_hello();
    } else if (remaining.ok() == 0) {
      // Parsing complete.
      handle_client_hello();
    } else {
      // Need to read again (unlikely).
      read_client_hello();
    }
  }
}

void tls_service::handle_not_client_hello() {
  // This is NOT a Client Hello message, so TLS is the wrong protocol to use.
  // Thus, we forward the data to a TCP tunnel.
  owner_.switch_service<tunnel::tunnel_service>();
}

void tls_service::handle_client_hello() {
  if (result<handshake::client_hello> message =
          handshake::client_hello::from_raw_data(client_hello_reader_.get_bytes());
      !message.is_ok()) {
    handle_not_client_hello();
    return;
  } else {
    client_hello_msg_ = std::make_unique<handshake::client_hello>(std::move(message).ok());
  }
  connect_server();
}

void tls_service::connect_server() { connect_server_async(std::bind_front(&tls_service::on_connect_server, this)); }

void tls_service::on_connect_server(const boost::system::error_code& error) {
  if (error != boost::system::errc::success) {
    flow_.error.set_boost_error(error);
    flow_.error.set_proxy_error(errc::upstream_connect_error);
    flow_.error.set_message(
        out::string::stream("Could not connect to ", flow_.server.host(), ':', flow_.server.port()));
    interceptors_.tls.run(intercept::tls_event::error, flow_);
    // Establish TLS with client and report the error later.
    establish_tls_with_client();
  } else {
    establish_tls_with_server();
  }
}

void tls_service::establish_tls_with_server() {
  if (result<void> res = establish_tls_with_server_impl(); !res.is_ok()) {
    flow_.error = std::move(res).err();
    stop();
  }
}

result<void> tls_service::establish_tls_with_server_impl() {
  if (client_hello_msg_ == nullptr) {
    return error::tls::tls_service_error("Must parse Client Hello message before establishing TLS with server");
  }

  auto method = options_.ssl_server_method;
  ssl_client_context_args_ = std::make_unique<openssl::ssl_context_args>(
      openssl::ssl_context_args{options_.ssl_verify, method, openssl::ssl_context_args::get_options_for_method(method),
                                std::string(client_store_.cert_file())});

  if (client_hello_msg_->has_alpn_extension() && !options_.ssl_negotiate_alpn) {
    // Remove unsupported protocols to be sure the server picks one we can read.
    std::copy_if(client_hello_msg_->alpn.begin(), client_hello_msg_->alpn.end(),
                 std::back_inserter(ssl_client_context_args_->alpn_protos),
                 [](const std::string& protocol) { return !((protocol.rfind("h2-") == 0) || (protocol == "SPDY")); });
    // TODO: Remove this line once HTTP 2 is supported.
    ssl_client_context_args_->alpn_protos.erase(
        std::remove(ssl_client_context_args_->alpn_protos.begin(), ssl_client_context_args_->alpn_protos.end(), "h2"),
        ssl_client_context_args_->alpn_protos.end());
  } else {
    // Set default ALPN.
    ssl_client_context_args_->alpn_protos.emplace_back(tls_service::default_alpn);
  }

  // If client TLS is established already, use client's negotiated ALPN by default.
  if (flow_.client.secured()) {
    ssl_client_context_args_->alpn_protos.clear();
    ssl_client_context_args_->alpn_protos.emplace_back(flow_.client.alpn());
  }

  if (!options_.ssl_negotiate_ciphers) {
    // Use only ciphers we have named with the server.
    std::copy_if(client_hello_msg_->cipher_suites.begin(), client_hello_msg_->cipher_suites.end(),
                 std::back_inserter(ssl_client_context_args_->cipher_suites),
                 [](const handshake::cipher_suite_name& cipher) { return handshake::cipher_is_valid(cipher); });
  }

  return flow_.establish_tls_with_server_async(*ssl_client_context_args_,
                                               std::bind_front(&tls_service::on_establish_tls_with_server, this));
}

void tls_service::on_establish_tls_with_server(const boost::system::error_code& error) {
  if (error != boost::system::errc::success) {
    flow_.error.set_boost_error(error);
    flow_.error.set_proxy_error(errc::upstream_handshake_failed);
    flow_.error.set_message(
        out::string::stream("Could not establish TLS with ", flow_.server.host(), ':', flow_.server.port()));
    interceptors_.tls.run(intercept::tls_event::error, flow_);
    // Establish TLS with client and report the error later.
  }

  establish_tls_with_client();
}

bool accept_all(bool, boost::asio::ssl::verify_context& context) { return true; }

int alpn_select_callback(SSL* ssl, const unsigned char** out, unsigned char* outlen, const unsigned char* in,
                         unsigned int inlen, void* arg) {
  // in contains protos to select from.
  // We must set out and outlen to the select ALPN.

  std::string_view protos = {reinterpret_cast<const char*>(in), inlen};
  std::size_t pos = 0;

  // Use ALPN already negotiated
  if (arg != nullptr) {
    std::string_view server_alpn = reinterpret_cast<const char*>(arg);
    if ((pos = protos.find(server_alpn)) != std::string::npos) {
      *out = in + pos;
      *outlen = static_cast<unsigned int>(server_alpn.length());
      return SSL_TLSEXT_ERR_OK;
    }
  }

  // Use default ALPN
  if ((pos = protos.find(tls_service::default_alpn)) != std::string::npos) {
    *out = in + pos;
    *outlen = static_cast<unsigned int>(tls_service::default_alpn.length());
  }
  // Use first option
  else {
    *outlen = static_cast<unsigned int>(*in);
    *out = in + 1;
  }

  return SSL_TLSEXT_ERR_OK;
}

void tls_service::establish_tls_with_client() {
  if (result<void> res = establish_tls_with_client_impl(); !res.is_ok()) {
    flow_.error = std::move(res).err();
    stop();
  }
}

result<void> tls_service::establish_tls_with_client_impl() {
  ASSIGN_OR_RETURN(x509::memory_certificate cert, get_certificate_for_client());
  auto method = options_.ssl_client_method;
  ssl_server_context_args_ = std::make_unique<openssl::ssl_server_context_args>(openssl::ssl_server_context_args{
      openssl::ssl_context_args{
          boost::asio::ssl::verify_none,
          method,
          openssl::ssl_context_args::get_options_for_method(method),
          cert.chain_file,
          accept_all,
          std::vector<handshake::cipher_suite_name>(default_client_ciphers.begin(), default_client_ciphers.end()),
          {},
          alpn_select_callback,
          flow_.server.secured() ? std::string(flow_.server.alpn()) : std::optional<std::string>{}},
      cert.cert, cert.pkey, server_store_.dhpkey()});

  if (options_.ssl_supply_server_chain_to_client && flow_.server.connected() && flow_.server.secured()) {
    ssl_server_context_args_->cert_chain = flow_.server.get_cert_chain();
  }

  return flow_.establish_tls_with_client_async(*ssl_server_context_args_,
                                               std::bind_front(&tls_service::on_establish_tls_with_client, this));
}

result<x509::memory_certificate> tls_service::get_certificate_for_client() {
  // Information that may be needed for the client certificate.
  x509::certificate_interface cert_interface;

  if (flow_.server.connected()) {
    // Always use host as the common name.
    cert_interface.common_name = flow_.server.host();

    // TLS is established, use certificate data.
    if (flow_.server.secured()) {
      auto& cert = flow_.server.cert();
      auto cert_sans = cert.sans();
      std::copy(cert_sans.begin(), cert_sans.end(), std::inserter(cert_interface.sans, cert_interface.sans.end()));

      ASSIGN_OR_RETURN(auto cn,  cert.common_name());
      if (cn.has_value()) {
        cert_interface.sans.insert(cn.value());
      }

      ASSIGN_OR_RETURN(auto org, cert.organization());
      if (org.has_value()) {
        cert_interface.organization = org.value();
      }
    }
  }

  // Copy Client Hello SNI entries.
  for (const auto& san : client_hello_msg_->server_names) {
    cert_interface.sans.insert(san.host_name);
  }

  if (cert_interface.common_name.has_value()) {
    cert_interface.sans.insert(cert_interface.common_name.value());
  }

  // Allow this stage to be intercepted, which is a bit dangerous!
  interceptors_.ssl_certificate.run(intercept::ssl_certificate_event::search, flow_, cert_interface);

  auto existing_cert = server_store_.get_certificate(cert_interface);
  if (existing_cert.has_value()) {
    return existing_cert.value();
  }

  interceptors_.ssl_certificate.run(intercept::ssl_certificate_event::create, flow_, cert_interface);

  return server_store_.create_certificate(cert_interface);
}

void tls_service::on_establish_tls_with_client(const boost::system::error_code& error) {
  if (error != boost::system::errc::success) {
    // This function helps debugging internal OpenSSL errors.
    // ERR_print_errors_fp(stdout);

    // Client is not going to continue if its handshake failed.
    flow_.error.set_boost_error(error);
    flow_.error.set_proxy_error(errc::downstream_handshake_failed);
    interceptors_.tls.run(intercept::tls_event::error, flow_);
    stop();
  } else {
    // TLS is successfully established within the connection objects.
    interceptors_.tls.run(intercept::tls_event::established, flow_);
    std::string_view alpn = flow_.client.alpn();
    if (alpn == "http/1.1" || alpn.empty()) {
      // Any TLS errors are reported to the client by the HTTP service.
      owner_.switch_service<http::http1::http_service>();
    } else if (!flow_.error.has_error()) {
      owner_.switch_service<tunnel::tunnel_service>();
    } else {
      stop();
    }
  }
}

}  // namespace proxy::tls
