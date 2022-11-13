/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>
#include <string>

#include "aether/proxy/connection/base_connection.hpp"
#include "aether/proxy/tcp/tls/x509/certificate.hpp"
#include "aether/proxy/types.hpp"

namespace proxy::connection {

// A connection to the client (whoever initiated the request).
class client_connection : public base_connection {
 public:
  client_connection(boost::asio::io_context& ioc, server_components& components);

  void establish_tls_async(tcp::tls::openssl::ssl_server_context_args& args, const err_callback_t& handler);

 private:
  void on_handshake(const boost::system::error_code& err, const err_callback_t& handler);

  std::string sni_;
  std::string cipher_name_;
  tcp::tls::openssl::ssl_method ssl_method_;
};

}  // namespace proxy::connection
