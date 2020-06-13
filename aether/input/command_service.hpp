/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>
#include <map>
#include <limits>
#include <thread>

#include <aether/proxy/server.hpp>
#include <aether/proxy/config/config.hpp>
#include <aether/input/types.hpp>
#include <aether/util/console.hpp>
#include <aether/util/string.hpp>

namespace input {
    namespace commands {
        class base_command;
    }

    class command_inserter;

    /*
        Service for running commands to interact with the running proxy server.
    */
    class command_service {
    private:
        static command_map_t command_map;

        // Responsible for inserting commands into the command map
        static command_inserter inserter;

        static constexpr std::string_view default_prefix = "aether/command > ";
        // Stream to read commands from
        std::istream &strm;

        // Needs access to the server object to add necessary interceptors
        proxy::server &server;

        std::string prefix;

        void command_loop();
        std::string read_command();
        arguments read_arguments();
        void discard_line();
        void run_command(commands::base_command &cmd, const arguments &args);

    public:
        command_service(std::istream &strm, proxy::server &server);
        
        /*
            Runs the command service until an exit condition is met.
        */
        void run();

        /*
            Tells the command service to stop after the current command.
        */
        void stop();

        /*
            Prints the opening line, which gives instructions on how to use the service.
        */
        void print_opening_line();
        
        const command_map_t get_commands() const;

        friend class command_inserter;
    };
}