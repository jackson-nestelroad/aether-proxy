/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/input/commands/base_command.hpp>
#include <aether/util/console.hpp>

namespace input::commands {
    /*
        Command that displays details on one or all other commands.
    */
    class help
        : public base_command {
    public:
        void run(const arguments &args, proxy::server &server, command_service &caller) override;

        inline std::string name() const override {
            return "help";
        }
        inline std::string args() const override {
            return "(cmd)";
        }
        inline std::string description() const override {
            return "Returns details about all (or one) commands.";
        }
        inline bool uses_signals() const override {
            return false;
        }
    };
}
