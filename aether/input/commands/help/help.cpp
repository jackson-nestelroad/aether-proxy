/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "help.hpp"

namespace input::commands {
    void help::run(const arguments &args, proxy::server &server, command_service &owner) {
        const std::string &lookup = args[0];
        const auto &commands = owner.get_commands();
        if (lookup.empty()) {
            for (const auto &[name, command] : commands) {
                out::console::log(command->name(), command->args());
                out::console::log(command->description(), out::manip::endl);
            }
        }
        else {
            auto it = commands.find(lookup);
            if (it != commands.end()) {
                auto &command = it->second;
                out::console::log(command->name(), command->args());
                out::console::log(command->description(), out::manip::endl);
            }
            else {
                out::console::stream("Could not find command `", lookup, "`. Use `help` for a list of commands.", out::manip::endl);
            }
        }
    }
}