/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "method.hpp"

#include <boost/lexical_cast.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>

#include "aether/proxy/error/exceptions.hpp"

namespace proxy::http {

namespace {
struct method_map : public std::unordered_map<std::string_view, method> {
  method_map() {
#define X(name) this->operator[](#name) = method::name;
    HTTP_METHODS(X)
#undef X
  }
};
}  // namespace

std::string_view method_to_string(method m) {
  switch (m) {
#define X(name)      \
  case method::name: \
    return #name;
    HTTP_METHODS(X)
#undef X
  }
  throw error::http::invalid_method_exception{};
}

method string_to_method(std::string_view str) {
  static method_map map;
  auto ptr = map.find(str);
  if (ptr == map.end()) {
    throw error::http::invalid_method_exception{};
  }
  return ptr->second;
}

std::ostream& operator<<(std::ostream& output, method m) {
  output << method_to_string(m);
  return output;
}

std::istream& operator>>(std::istream& input, method& m) {
  std::string str;
  input >> str;
  m = string_to_method(str);
  return input;
}

}  // namespace proxy::http
