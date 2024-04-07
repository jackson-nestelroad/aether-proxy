/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "version.hpp"

#include <boost/lexical_cast.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>

#include "aether/proxy/error/error.hpp"

namespace proxy::http {

namespace {

struct version_map : public std::unordered_map<std::string_view, version> {
  version_map() {
#define X(name, string) this->operator[](string) = version::name;
    HTTP_VERSIONS(X)
#undef X
  }
};

}  // namespace

result<std::string_view> version_to_string(version v) {
  switch (v) {
#define X(name, string) \
  case version::name:   \
    return string;
    HTTP_VERSIONS(X)
#undef X
  }
  return error::http::invalid_version();
}

result<version> string_to_version(std::string_view str) {
  static version_map map;
  auto ptr = map.find(str);
  if (ptr == map.end()) {
    return error::http::invalid_version();
  }
  return ptr->second;
}

std::ostream& operator<<(std::ostream& output, version v) {
  output << version_to_string(v);
  return output;
}

}  // namespace proxy::http
