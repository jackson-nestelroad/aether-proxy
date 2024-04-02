/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>
#include <string_view>

#include "aether/proxy/error/error.hpp"
#include "aether/proxy/error/exceptions.hpp"

#define HTTP_VERSIONS(X) \
  X(http1_0, "HTTP/1.0") \
  X(http1_1, "HTTP/1.1") \
  X(http2_0, "HTTP/2.0") \
  X(http3_0, "HTTP/3.0")

namespace proxy::http {

// Enumeration type for versions of HTTP.
enum class version {
#define X(name, string) name,
  HTTP_VERSIONS(X)
#undef X
};

// Converts an HTTP version to string.
result<std::string_view> version_to_string(version v);

// Converts a string to an HTTP version.
result<version> string_to_version(std::string_view str);

std::ostream& operator<<(std::ostream& output, version v);

}  // namespace proxy::http
