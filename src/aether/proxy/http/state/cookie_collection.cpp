/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "cookie_collection.hpp"

#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

#include "aether/proxy/http/state/cookie.hpp"

namespace proxy::http {

void cookie_collection::update(const cookie_collection& new_cookies) {
  for (const auto& [name, new_cookie] : new_cookies) {
    set(new_cookie);
  }
}

std::optional<std::reference_wrapper<cookie>> cookie_collection::get(std::string_view name) {
  auto it = cookies_.find(name);
  if (it == cookies_.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<std::reference_wrapper<const cookie>> cookie_collection::get(std::string_view name) const {
  auto it = cookies_.find(name);
  if (it == cookies_.end()) {
    return std::nullopt;
  }
  return it->second;
}

void cookie_collection::set(const cookie& cook) {
  auto it = cookies_.find(cook.name);
  if (it == cookies_.end()) {
    cookies_.emplace(cook.name, cook);
  } else {
    it->second = cook;
  }
}

void cookie_collection::remove(const cookie& cook) { cookies_.erase(cook.name); }

std::size_t cookie_collection::size() const { return cookies_.size(); }

bool cookie_collection::empty() const { return cookies_.empty(); }

std::string cookie_collection::request_string() const {
  if (cookies_.empty()) {
    return "";
  }

  std::ostringstream str;
  auto it = cookies_.begin();
  str << it->second.request_string();

  for (++it; it != cookies_.end(); ++it) {
    str << "; " << it->second.request_string();
  }
  return str.str();
}

cookie_collection cookie_collection::parse_request_header(std::string_view header) {
  cookie_collection result;
  for (std::size_t i = 0; i < header.length();) {
    ++i;
    std::size_t separator = header.find('=', i);
    std::size_t cookie_end = header.find(';', i);
    if (separator < cookie_end) {
      result.set({util::string::trim(util::string::substring(header, i, separator)),
                  util::string::trim(util::string::substring(header, separator + 1, cookie_end))});
    }
    i = cookie_end;
  }
  return result;
}

}  // namespace proxy::http
