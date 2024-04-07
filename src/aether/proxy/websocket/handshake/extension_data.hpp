/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "aether/proxy/error/error.hpp"

namespace proxy::websocket::handshake {

// Represents the data for a single WebSocket extension.
class extension_data {
 public:
  static constexpr char extension_delim = ',';
  static constexpr char param_delim = ';';
  static constexpr char assign_delim = '=';

  extension_data(std::string name);
  extension_data() = delete;
  ~extension_data() = default;
  extension_data(const extension_data& other) = delete;
  extension_data& operator=(const extension_data& other) = delete;
  extension_data(extension_data&& other) noexcept = default;
  extension_data& operator=(extension_data&& other) noexcept = default;

  inline const std::string& name() const { return name_; }
  inline void set_name(std::string name) { name_ = std::move(name); }

  bool has_param(std::string_view name) const;
  result<std::string_view> get_param(std::string_view name) const;
  void set_param(const std::string& name, std::string_view value = "");

  // Parses a single extension string and its parameters.
  //
  // The input string must not have an extension::extension_delim character anywhere in it, as this means there are two
  // extensions within the string.
  static result<extension_data> from_header_value(std::string_view header);

 private:
  std::string name_;
  std::map<std::string, std::string, std::less<>> params_;

  friend std::ostream& operator<<(std::ostream& out, const extension_data& ext);
  friend std::ostream& operator<<(std::ostream& out, const std::vector<extension_data>& ext_list);
};
}  // namespace proxy::websocket::handshake
