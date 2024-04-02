/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "ssl_method.hpp"

#include <boost/lexical_cast.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>

#include "aether/proxy/error/error.hpp"

namespace proxy::tls::openssl {

result<std::string_view> ssl_method_to_string(ssl_method method) {
  switch (method) {
#define X(name, str) \
  case name:         \
    return str;
    SSL_METHODS(X)
#undef X
    default:
      break;
  }
  return error::tls::invalid_ssl_method();
}

struct ssl_method_map
    : public std::unordered_map<std::string_view, ssl_method, util::string::ihash, util::string::iequals> {
  ssl_method_map() {
#define X(name, str) this->operator[](str) = name;
    SSL_METHODS(X)
#undef X
  }
};

result<ssl_method> string_to_ssl_method(std::string_view str) {
  static ssl_method_map map;
  auto ptr = map.find(str);
  if (ptr == map.end()) {
    return error::tls::invalid_ssl_method();
  }
  return ptr->second;
}

}  // namespace proxy::tls::openssl

namespace boost::asio::ssl {

std::ostream& operator<<(std::ostream& output, context::method m) {
  return output << ::proxy::tls::openssl::ssl_method_to_string(m);
}

}  // namespace boost::asio::ssl
