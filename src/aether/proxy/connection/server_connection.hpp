/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>
#include <string>
#include <string_view>

#include "aether/proxy/connection/base_connection.hpp"
#include "aether/proxy/tls/x509/certificate.hpp"
#include "aether/proxy/types.hpp"

namespace proxy::connection {

// A connection to the server (wherever the client specifies).
class server_connection : public base_connection {
 public:
  server_connection(boost::asio::io_context& ioc, server_components& components);

  void connect_async(std::string host, port_t port, err_callback_t handler);
  void establish_tls_async(tls::openssl::ssl_context_args& args, err_callback_t handler);

  inline std::string_view host() const { return host_; }
  inline port_t port() const { return port_; }

  inline bool is_connected_to(std::string_view host, port_t port) const {
    return connected() && host_ == host && port_ == port;
  }

  inline const std::vector<tls::x509::certificate>& get_cert_chain() const { return cert_chain_; }

 private:
  void on_resolve(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
                  err_callback_t handler);
  void on_connect(const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
                  err_callback_t handler);
  void on_handshake(const boost::system::error_code& err, err_callback_t handler);

  boost::asio::ip::tcp::resolver resolver_;
  boost::asio::ip::tcp::endpoint endpoint_;

  std::string host_;
  port_t port_;

  std::vector<tls::x509::certificate> cert_chain_;
};

}  // namespace proxy::connection
