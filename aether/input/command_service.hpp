/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>
#include <map>
#include <limits>

#include <aether/proxy/server.hpp>
#include <aether/proxy/config/config.hpp>
#include <aether/input/commands/base_command.hpp>
#include <aether/util/console.hpp>

namespace input {
    class command_service {
    private:
        using arguments = std::vector<std::string>;
        using command_map_t = std::map<std::string, std::shared_ptr<commands::base_command>>;

        static command_map_t command_map;

        static constexpr std::string_view default_prefix = "aether/command > ";
        // Stream to read commands from
        std::istream &strm;

        // Needs access to the server object to add necessary interceptors
        proxy::server &server;

        std::string prefix;

        std::string read_command();
        arguments read_arguments();
        void discard_line();
        void help(const arguments &args);
        void print_opening_line();

    public:
        command_service(std::istream &strm, proxy::server &server);
        
        /*
            Runs the command service until an exit condition is met.
        */
        void run();
        
        /*
            Inserts a new command into all command service instances.
        */
        template <typename T>
        std::enable_if_t<std::is_base_of_v<commands::base_command, T>, void> 
        insert_command() {
            auto new_command = T();
            command_map.emplace(new_command.name(), new_command);
        }
    };
}