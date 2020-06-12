/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <string>
#include <vector>

#include <aether/proxy/server.hpp>
#include <aether/util/console.hpp>
#include <aether/input/command_service.hpp>

namespace input::commands {
    /*
        Interface to be implemented by all commands.
    */
    class base_command {
    public:
        virtual void run(const arguments &args, proxy::server &server, command_service &caller) = 0;
        virtual std::string name() const = 0;
        virtual std::string args() const = 0;
        virtual std::string description() const = 0;
        virtual bool uses_signals() const;
    };
}