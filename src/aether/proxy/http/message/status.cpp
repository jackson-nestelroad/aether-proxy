/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "status.hpp"

#include <boost/lexical_cast.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>

#include "aether/proxy/error/exceptions.hpp"

namespace proxy::http {

std::string_view status_to_reason(status s) {
  switch (s) {
#define X(num, name, string) \
  case status::name:         \
    return string;
    HTTP_STATUS_CODES(X)
#undef X
  }
  throw error::http::invalid_status_exception{};
}

status string_to_status(std::string_view str) {
  std::size_t code = 500;
  try {
    code = boost::lexical_cast<std::size_t>(str);
  } catch (const boost::bad_lexical_cast&) {
    // Allow invalid HTTP statuses.
  }
  return code_to_status(code);
}

status code_to_status(std::size_t code) {
  // Allow invalid HTTP statuses.
  return static_cast<status>(code);
}

std::ostream& operator<<(std::ostream& output, status s) {
  output << static_cast<std::size_t>(s);
  return output;
}

std::istream& operator>>(std::istream& input, status& s) {
  std::string str;
  input >> str;
  s = string_to_status(str);
  return input;
}

}  // namespace proxy::http
