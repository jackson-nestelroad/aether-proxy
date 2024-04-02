/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>
#include <string>

#include "aether/proxy/connection/base_connection.hpp"
#include "aether/proxy/error/error.hpp"
#include "aether/proxy/tls/x509/certificate.hpp"
#include "aether/proxy/types.hpp"

namespace proxy::connection {

// A connection to the client (whoever initiated the request).
class client_connection : public base_connection {
 public:
  client_connection(boost::asio::io_context& ioc, server_components& components);

  result<void> establish_tls_async(tls::openssl::ssl_server_context_args& args, err_callback_t handler);

 private:
  result<void> on_handshake(err_callback_t handler, const boost::system::error_code& error);

  std::string sni_;
  std::string cipher_name_;
  tls::openssl::ssl_method ssl_method_;
};

}  // namespace proxy::connection
