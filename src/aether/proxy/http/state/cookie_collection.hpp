/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "aether/proxy/http/state/cookie.hpp"

namespace proxy::http {

// Class representing a collection of cookies.
class cookie_collection {
 public:
  cookie_collection() = default;
  ~cookie_collection() = default;
  cookie_collection(const cookie_collection& other) = default;
  cookie_collection& operator=(const cookie_collection& other) = default;
  cookie_collection(cookie_collection&& other) noexcept = default;
  cookie_collection& operator=(cookie_collection&& other) noexcept = default;

  void update(const cookie_collection& new_cookies);
  std::optional<std::reference_wrapper<cookie>> get(std::string_view name);
  std::optional<std::reference_wrapper<const cookie>> get(std::string_view name) const;
  void set(const cookie& cook);
  void remove(const cookie& cook);

  bool empty() const;
  std::size_t size() const;
  std::string request_string() const;

  inline auto begin() { return cookies_.begin(); }
  inline auto end() { return cookies_.end(); }
  inline auto begin() const { return cookies_.begin(); }
  inline auto end() const { return cookies_.end(); }

  static cookie_collection parse_request_header(std::string_view header);

 private:
  std::map<std::string, cookie, std::less<>> cookies_;
};
}  // namespace proxy::http
