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

#include "aether/proxy/error/error.hpp"
#include "aether/util/string.hpp"

namespace proxy::tls::openssl {

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

result<std::string_view> ssl_method_to_string(ssl_method method);

result<ssl_method> string_to_ssl_method(std::string_view str);

}  // namespace proxy::tls::openssl

namespace boost::asio::ssl {

std::ostream& operator<<(std::ostream& output, context::method m);

}  // namespace boost::asio::ssl
