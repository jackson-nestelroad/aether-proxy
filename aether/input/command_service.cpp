/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "command_service.hpp"

namespace input {
    // Insert default commands here
    command_service::command_map_t command_service::command_map {

    };
    
    command_service::command_service(std::istream &strm, proxy::server &server)
        : strm(strm),
        server(server),
        prefix(default_prefix)
    { }

    void command_service::run() {
        print_opening_line();
        while (true) {
            out::console::stream(prefix);
            std::string cmd = read_command();
            if (cmd == "help") {
                arguments args = read_arguments();
                help(args);
            }
            else if (cmd == "stop") {
                discard_line();
                break;
            }
            else {
                auto it = command_map.find(cmd);
                if (it != command_map.end()) {
                    arguments args = read_arguments();
                    it->second->run(args);
                }
                else {
                    out::error::stream("Invalid command `", cmd, "`. Use `help` for a list of commands.", out::manip::endl);
                    discard_line();
                }
            }
        }
    }

    std::string command_service::read_command() {
        std::string cmd;
        strm >> cmd;
        return cmd;
    }

    command_service::arguments command_service::read_arguments() {
        std::string args_string;
        std::getline(strm, args_string);
        return util::string::split(args_string, " ");
    }

    void command_service::discard_line() {
        strm.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    void command_service::print_opening_line() {
        out::console::log("Use `help` for a list of commands. Use `stop` to stop the server.", out::manip::endl);
    }

    void command_service::help(const arguments &args) {
        const std::string &lookup = args[0];
        if (lookup.empty()) {
            print_opening_line();
            for (const auto &[name, command] : command_map) {
                out::console::log(command->name(), command->args());
                out::console::log(command->description(), out::manip::endl);
            }
        }
        else {
            auto it = command_map.find(lookup);
            if (it != command_map.end()) {
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