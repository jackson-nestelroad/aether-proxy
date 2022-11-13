/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "cookie.hpp"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <locale>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

#include "aether/util/string.hpp"

namespace proxy::tcp::http {

const std::locale cookie::locale(std::locale::classic(),
                                 new boost::posix_time::time_input_facet("%a, %d %b %Y %H:%M:%S %Z"));
const boost::posix_time::ptime cookie::epoch(boost::gregorian::date(1970, 1, 1));

cookie::cookie(std::string_view name, std::string_view value) : name(name), value(value) {}

std::optional<cookie> cookie::parse_set_header(std::string_view header) {
  std::size_t separator = header.find('=');
  if (separator == std::string::npos) {
    return std::nullopt;
  }
  std::size_t attribute_separator = header.find(';');
  cookie result = {util::string::trim(util::string::substring(header, 0, separator)),
                   util::string::trim(util::string::substring(header, separator + 1, attribute_separator))};

  if (attribute_separator != std::string::npos) {
    for (std::size_t i = attribute_separator; i < header.length();) {
      ++i;
      std::size_t attribute_end = header.find(';', i);
      std::size_t separator = header.find('=', i);

      // No value
      if (separator >= attribute_end) {
        result.attributes_.emplace(util::string::trim(util::string::substring(header, i, attribute_end)), "");
      } else {
        result.attributes_.emplace(util::string::trim(util::string::substring(header, i, separator)),
                                   util::string::trim(util::string::substring(header, separator + 1, attribute_end)));
      }

      i = attribute_end;
    }
  }

  return result;
}

std::optional<std::string> cookie::get_attribute(std::string_view attribute) const {
  auto it = attributes_.find(attribute);
  if (it == attributes_.end()) {
    return std::nullopt;
  }
  return it->second;
}
void cookie::set_attribute(std::string_view attribute, std::string_view value) {
  auto it = attributes_.find(attribute);
  if (it == attributes_.end()) {
    attributes_.emplace(attribute, value);
  } else {
    it->second = value;
  }
}

std::optional<boost::posix_time::ptime> cookie::expires() const {
  auto it = attributes_.find("Expires");
  if (it == attributes_.end()) {
    return std::nullopt;
  }

  std::istringstream strm;
  strm.imbue(locale);
  boost::posix_time::ptime result;
  strm >> result;

  if (result == boost::posix_time::ptime()) {
    return std::nullopt;
  }
  return result;
}

std::optional<std::string> cookie::domain() const {
  auto it = attributes_.find("Domain");
  if (it == attributes_.end()) {
    return std::nullopt;
  }

  std::size_t start = 0;
  if (it->second[0] == '.') {
    ++start;
  }
  return it->second.substr(start);
}

void cookie::expire() { set_expires(epoch); }

void cookie::set_expires(const boost::posix_time::ptime& time) {
  std::ostringstream strm;
  strm.imbue(locale);
  strm << time;
  set_attribute("Expires", strm.str());
}

void cookie::set_domain(std::string_view domain) { set_attribute("Domain", domain); }

std::string cookie::request_string() const { return name + '=' + value; }

std::string cookie::response_string() const {
  std::stringstream str;
  str << *this;
  return str.str();
}

bool cookie::operator<(const cookie& other) const { return name < other.name; }

std::ostream& operator<<(std::ostream& output, const cookie& cookie) {
  output << cookie.name << '=' << cookie.value;
  for (const auto& attribute_pair : cookie.attributes_) {
    output << "; " << attribute_pair.first;
    if (!attribute_pair.second.empty()) {
      output << '=' << attribute_pair.second;
    }
  }
  return output;
}

}  // namespace proxy::tcp::http
