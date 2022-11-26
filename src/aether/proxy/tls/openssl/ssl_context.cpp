/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "ssl_context.hpp"

#include <openssl/ssl.h>

#include <boost/asio/ssl.hpp>
#include <boost/lexical_cast.hpp>
#include <memory>
#include <string>

#include "aether/proxy/tls/handshake/handshake_types.hpp"
#include "aether/proxy/tls/openssl/openssl_ptrs.hpp"
#include "aether/proxy/tls/openssl/ssl_method.hpp"
#include "aether/proxy/tls/x509/certificate.hpp"
#include "aether/util/bytes.hpp"
#include "aether/util/string.hpp"

namespace proxy::tls::openssl {

std::string to_sni(const std::string& host, port_t port) { return host + ':' + boost::lexical_cast<std::string>(port); }

long ssl_context_args::get_options_for_method(ssl_method method) {
  if (method == ssl_method::sslv23) {
    return sslv23_options;
  } else {
    return default_options;
  }
}

std::unique_ptr<boost::asio::ssl::context> create_ssl_context(ssl_context_args& args) {
  auto ctx = std::make_unique<boost::asio::ssl::context>(args.method);
  ctx->set_verify_mode(args.verify);
  ctx->set_options(args.options);

  if (args.verify != boost::asio::ssl::context::verify_none) {
    ctx->load_verify_file(args.verify_file);
  }

  if (args.verify_callback.has_value()) {
    ctx->set_verify_callback(args.verify_callback.value());
  }

  SSL_CTX_set_mode(ctx->native_handle(), SSL_MODE_AUTO_RETRY);

  // SSL_CTX_set_security_level(ctx->native_handle(), 1);

  int res = 0;

  if (!args.cipher_suites.empty()) {
    res = SSL_CTX_set_cipher_list(ctx->native_handle(), util::string::join(args.cipher_suites, ":").c_str());
    if (res != 0) {
      throw error::tls::invalid_cipher_suite_list_exception{};
    }
  }

  if (!args.alpn_protos.empty()) {
    auto wire_alpn = util::bytes::to_wire_format<1>(args.alpn_protos);
    res = SSL_CTX_set_alpn_protos(ctx->native_handle(), wire_alpn.data(), static_cast<unsigned int>(wire_alpn.size()));
    if (res != 0) {
      throw error::tls::invalid_alpn_protos_list_exception{};
    }
  }

  if (args.alpn_select_callback.has_value()) {
    SSL_CTX_set_alpn_select_cb(ctx->native_handle(), args.alpn_select_callback.value(),
                               args.server_alpn.has_value() && args.server_alpn.value().length() > 0
                                   ? args.server_alpn.value().data()
                                   : nullptr);
  }

  return ctx;
}

void enable_hostname_verification(boost::asio::ssl::context& ctx, const std::string& sni) {
  // Manually enable hostname verification.
  int res = 0;
  X509_VERIFY_PARAM* param = X509_VERIFY_PARAM_new();
  res += X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_CRL_CHECK);
  X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS | X509_CHECK_FLAG_NEVER_CHECK_SUBJECT);
  res += X509_VERIFY_PARAM_set1_host(param, sni.c_str(), sni.size());
  res += SSL_CTX_set1_param(ctx.native_handle(), param);
  X509_VERIFY_PARAM_free(param);

  if (res != 3) {
    throw error::tls::ssl_context_error_exception{"Failed to enable hostname verification"};
  }
}

}  // namespace proxy::tls::openssl
