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

#include "aether/proxy/error/exceptions.hpp"

namespace proxy::tcp::tls::openssl {

struct ssl_method_map
    : public std::unordered_map<std::string_view, ssl_method, util::string::ihash, util::string::iequals> {
  ssl_method_map() {
#define X(name, str) this->operator[](str) = name;
    SSL_METHODS(X)
#undef X
  }
};

ssl_method string_to_ssl_method(std::string_view str) {
  static ssl_method_map map;
  auto ptr = map.find(str);
  if (ptr == map.end()) {
    throw error::tls::invalid_ssl_method_exception{};
  }
  return ptr->second;
}

}  // namespace proxy::tcp::tls::openssl

namespace boost::asio::ssl {

std::ostream& operator<<(std::ostream& output, context::method m) {
  return output << ::proxy::tcp::tls::openssl::ssl_method_to_string(m);
}

std::istream& operator>>(std::istream& input, context::method& m) {
  std::string str;
  input >> str;
  m = ::proxy::tcp::tls::openssl::string_to_ssl_method(str);
  return input;
}

}  // namespace boost::asio::ssl
