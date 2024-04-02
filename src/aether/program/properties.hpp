/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

#include "aether/util/generic_error.hpp"
#include "aether/util/result.hpp"

namespace program {

// Parser and reader for a file of key-value pairs.
class properties {
 public:
  // Get the value associated to a property key.
  std::optional<std::string_view> get(std::string_view key) const;

  // Parse the values of a flat .properties file.
  util::result<void, util::generic_error> parse_file(std::string_view file_path);

 private:
  std::unordered_map<std::string, std::string> props_;
};

}  // namespace program
