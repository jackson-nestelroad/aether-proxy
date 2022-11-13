/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "properties.hpp"

#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "aether/util/console.hpp"
#include "aether/util/string.hpp"

namespace program {
std::optional<std::string_view> properties::get(std::string_view key) const {
  auto it = props_.find(key.data());
  if (it == props_.end()) {
    return std::nullopt;
  }
  return it->second;
}

void properties::parse_file(std::string_view file_path) {
  std::fstream file(file_path, std::fstream::in);
  if (!file) {
    throw properties_exception{out::string::stream("Could not open properties file \"", file_path, "\" for reading.")};
  }

  std::string line;
  while (std::getline(file, line)) {
    if (!line.empty() && line.at(0) != '#') {
      auto split_loc = line.find('=');
      if (split_loc == 0) {
        throw properties_exception{"Malformed property \"" + line + "\"."};
      }
      if (split_loc == std::string::npos) {
        throw properties_exception{"Property \"" + line + "\" does not have a value."};
      }
      props_.emplace(line.substr(0, split_loc), line.substr(split_loc + 1));
    }
  }
}

}  // namespace program
