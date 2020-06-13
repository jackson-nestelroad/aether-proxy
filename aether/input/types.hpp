/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <vector>
#include <string>
#include <functional>
#include <map>
#include <memory>

namespace input {
    namespace commands {
        class base_command;
    }

    using arguments = std::vector<std::string>;
    using command_map_t = std::map<std::string, std::shared_ptr<commands::base_command>>;
    using callback = std::function<void()>;
}