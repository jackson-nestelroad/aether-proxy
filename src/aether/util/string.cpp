/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "string.hpp"

#include <algorithm>
#include <istream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>

#include "aether/util/generic_error.hpp"
#include "aether/util/result.hpp"

namespace util::string {

std::string_view trim(std::string_view src, std::string_view whitespace) {
  std::size_t begin = src.find_first_not_of(whitespace);
  if (begin == std::string::npos) {
    return {};
  }
  std::size_t end = src.find_last_not_of(whitespace);
  return src.substr(begin, end - begin + 1);
}

std::string lowercase(std::string_view src) {
  std::string out;
  std::transform(src.begin(), src.end(), std::back_inserter(out), [](char c) { return std::tolower(c); });
  return out;
}

bool ends_with(std::string_view str, std::string_view suffix) {
  return str.size() >= suffix.size() &&
         std::equal(str.begin() + str.size() - suffix.size(), str.end(), suffix.begin(), suffix.end());
}

bool iequals_fn(std::string_view a, std::string_view b) {
  return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                    [](char a, char b) { return std::tolower(a) == std::tolower(b); });
}

bool iless_fn(std::string_view a, std::string_view b) {
  return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
                                      [](char a, char b) { return std::tolower(a) < std::tolower(b); });
}

std::size_t ihash::operator()(std::string_view s) const { return hasher_(lowercase(s)); }

result<std::size_t, generic_error> parse_hexadecimal(std::string_view src) {
  std::size_t out;
  std::stringstream ss;
  ss << std::hex << src;
  if (!(ss >> out) || !(ss >> std::ws).eof()) {
    return generic_error("String is not a hexadecimal integer");
  }
  return out;
}

}  // namespace util::string
