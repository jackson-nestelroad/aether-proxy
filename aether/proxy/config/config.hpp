/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <string_view>

#include <aether/util/string_view.hpp>

// General configuration data for the proxy server

namespace proxy::config {
    constexpr std::string_view _space = " ";
    constexpr std::string_view version = "1.0";
    constexpr std::string_view server_name = "Aether Proxy";
    constexpr std::string_view full_server_name = string_view::join_v<server_name, _space, version>;
}