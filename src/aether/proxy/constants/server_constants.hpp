/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <string_view>

#include "aether/util/string_view.hpp"

// General configuration data for the proxy server.

namespace proxy::constants {

inline constexpr std::string_view space = " ";
inline constexpr std::string_view version = "2.0";
inline constexpr std::string_view server_name = "Aether";

inline constexpr std::string_view lowercase_name = util::string_view::to_lowercase_v<server_name>;
inline constexpr std::string_view full_server_name = util::string_view::join_v<server_name, space, version>;

}  // namespace proxy::constants
