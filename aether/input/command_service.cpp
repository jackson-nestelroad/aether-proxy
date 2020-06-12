/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "command_service.hpp"

#include <aether/input/commands/base_command.hpp>
#include <aether/input/command_inserter.hpp>

namespace input {
    command_map_t command_service::command_map;
    command_inserter command_service::inserter;

    command_service::command_service(std::istream &strm, proxy::server &server)
        : strm(strm),
        server(server),
        ios(),
        work(ios),
        prefix(default_prefix),
        running(false),
        signals(ios)
    { }

    void command_service::run() {
        ios_runner = std::thread(
            [this]() {
                while (true) {
                    try {
                        ios.run();
                        break;
                    }
                    catch (const std::exception &ex) {
                        out::error::log(ex.what());
                    }
                }
            });
        running = true;
        signals.wait([this]() { running = false; });
        print_opening_line();
        command_loop();
    }

    void command_service::command_loop() {
        while (running) {
            out::console::stream(prefix);
            std::string cmd = read_command();

            auto it = command_map.find(cmd);
            if (it != command_map.end()) {
                auto args = read_arguments();
                if (it->second->uses_signals()) {
                    signals.pause();
                    run_command(*it->second, args);
                    signals.unpause();
                }
                else {
                    run_command(*it->second, args);
                }
            }
            else {
                out::error::stream("Invalid command `", cmd, "`. Use `help` for a list of commands.", out::manip::endl);
                discard_line();
            }
        }
        cleanup();
    }

    void command_service::run_command(commands::base_command &cmd, const arguments &args) {
        cmd.run(args, server, *this);
    }

    void command_service::cleanup() {
        ios.stop();
        ios_runner.join();
    }

    std::string command_service::read_command() {
        std::string cmd;
        if (!(strm >> cmd)) {
            running = false;
        }
        return cmd;
    }

    arguments command_service::read_arguments() {
        std::string args_string;
        if (strm.peek() == ' ') {
            strm.get();
        }
        std::getline(strm, args_string);
        return ::util::string::split(args_string, " ");
    }

    void command_service::discard_line() {
        strm.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    void command_service::print_opening_line() {
        out::console::log("Use `help` for a list of commands. Use `stop` to stop the server.", out::manip::endl);
    }

    void command_service::stop() {
        // Turn off running flag so the command_service will stop after the current command finishes
        running = false;
    }

    const command_map_t command_service::get_commands() const {
        return command_map;
    }

    boost::asio::io_service &command_service::io_service() {
        return ios;
    }
}