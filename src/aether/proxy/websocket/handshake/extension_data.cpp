/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "extension_data.hpp"

#include "aether/proxy/error/exceptions.hpp"
#include "aether/util/console.hpp"
#include "aether/util/string.hpp"

namespace proxy::websocket::handshake {

extension_data::extension_data(std::string name) : name_(std::move(name)) {}

bool extension_data::has_param(std::string_view name) const { return params_.find(name) != params_.end(); }

std::string_view extension_data::get_param(std::string_view name) const {
  const auto& it = params_.find(name);
  if (it == params_.end()) {
    throw error::websocket::extension_param_not_found_exception{
        out::string::stream("Extension parameter \"", name, "\" was not found")};
  }
  return it->second;
}

void extension_data::set_param(const std::string& name, std::string_view value) {
  params_.insert_or_assign(name, value);
}

extension_data extension_data::from_header_value(std::string_view header) {
  std::size_t multi_extensions = header.find_first_of(extension_delim);
  if (multi_extensions != std::string::npos) {
    throw error::websocket::invalid_extension_string_exception{"Multiple extensions present in single string"};
  }

  const auto& parsed = util::string::split_trim<std::string>(header, param_delim);
  std::size_t size = parsed.size();
  if (size < 1) {
    throw error::websocket::invalid_extension_string_exception{"No extension name found"};
  }

  extension_data result(parsed[0]);
  for (std::size_t i = 1; i < size; ++i) {
    const std::string& curr = parsed[i];
    std::size_t assign_pos = curr.find_first_of(assign_delim);
    if (assign_pos == std::string::npos) {
      result.set_param(curr);
    } else {
      result.set_param(curr.substr(0, assign_pos), curr.substr(assign_pos + 1));
    }
  }

  return result;
}

std::ostream& operator<<(std::ostream& out, const extension_data& ext) {
  out << ext.name();
  for (const auto& [param, value] : ext.params_) {
    out << extension_data::param_delim << ' ' << param << extension_data::assign_delim << value;
  }
  return out;
}

std::ostream& operator<<(std::ostream& out, const std::vector<extension_data>& ext_list) {
  bool first = true;
  for (const auto& ext : ext_list) {
    if (first) {
      out << ext;
      first = false;
    } else {
      out << extension_data::extension_delim << ' ' << ext;
    }
  }
  return out;
}

}  // namespace proxy::websocket::handshake
