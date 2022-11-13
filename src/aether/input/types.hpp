/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace input {
namespace commands {
class base_command;
}

using arguments_t = std::vector<std::string>;
using command_map_t = std::map<std::string, std::shared_ptr<commands::base_command>, std::less<>>;
using callback_t = std::function<void()>;

}  // namespace input
