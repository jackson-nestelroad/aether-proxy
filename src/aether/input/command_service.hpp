/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <thread>

#include "aether/input/types.hpp"
#include "aether/proxy/server.hpp"

namespace input {
namespace commands {
class base_command;
}

class command_inserter;

// Service for running commands to interact with the running proxy server.
class command_service {
 public:
  command_service(std::istream& stream, proxy::server& server);

  // Runs the command service until an exit condition is met.
  void run();

  // Tells the command service to stop after the current command.
  void stop();

  // Prints the opening line, which gives instructions on how to use the service.
  void print_opening_line();

  const command_map_t& get_commands() const;

 private:
  static command_map_t command_map;
  static command_inserter inserter;

  static constexpr std::string_view default_prefix = "aether/command > ";

  void command_loop();
  std::string read_command();
  arguments_t read_arguments();
  void discard_line();
  void run_command(commands::base_command& cmd, const arguments_t& args);

  // Stream to read commands from.
  std::istream& stream_;

  // Needs access to the server object to add necessary interceptors.
  proxy::server& server_;

  std::string prefix_;

  friend class command_inserter;
};

}  // namespace input
