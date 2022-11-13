/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio/ssl.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>

#include "aether/proxy/error/exceptions.hpp"
#include "aether/util/string.hpp"

namespace proxy::tcp::tls::openssl {

// Allowed SSL methods.
#define SSL_METHODS(X)                            \
  X(boost::asio::ssl::context::sslv23, "SSLv23")  \
  X(boost::asio::ssl::context::sslv2, "SSLv2")    \
  X(boost::asio::ssl::context::sslv3, "SSLv3")    \
  X(boost::asio::ssl::context::tlsv1, "TLSv1")    \
  X(boost::asio::ssl::context::tlsv11, "TLSv1.1") \
  X(boost::asio::ssl::context::tlsv12, "TLSv1.2") \
  X(boost::asio::ssl::context::tlsv13, "TLSv1.3")

using ssl_method = boost::asio::ssl::context::method;

constexpr std::string_view ssl_method_to_string(ssl_method method) {
  switch (method) {
#define X(name, str) \
  case name:         \
    return str;
    SSL_METHODS(X)
#undef X
    default:
      break;
  }
  throw error::tls::invalid_ssl_method_exception{};
}

ssl_method string_to_ssl_method(std::string_view str);

}  // namespace proxy::tcp::tls::openssl

namespace boost::asio::ssl {

std::ostream& operator<<(std::ostream& output, context::method m);

std::istream& operator>>(std::istream& input, context::method& m);

}  // namespace boost::asio::ssl
