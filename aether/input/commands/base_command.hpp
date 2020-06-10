/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <string>
#include <vector>

#include <aether/util/console.hpp>

namespace input::commands {
    /*
        Interface to be implemented by all commands.
    */
    class base_command {
    public:
        virtual void run(const std::vector<std::string> &args) = 0;
        virtual std::string name() const = 0;
        virtual std::string args() const = 0;
        virtual std::string description() const = 0;
    };
}