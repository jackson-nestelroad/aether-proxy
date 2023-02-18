/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "command_service.hpp"

#include <iostream>
#include <limits>

#include "aether/input/command_inserter.hpp"
#include "aether/input/commands/base_command.hpp"
#include "aether/util/console.hpp"
#include "aether/util/string.hpp"

namespace input {

command_map_t command_service::command_map;
command_inserter command_service::inserter;

command_service::command_service(std::istream& stream, proxy::server& server)
    : stream_(stream), server_(server), prefix_(default_prefix) {}

void command_service::run() {
  print_opening_line();
  command_loop();
}

void command_service::command_loop() {
  while (server_.running()) {
    out::user::stream(prefix_);
    std::string cmd = read_command();

    // Signal was given, server likely exited.
    if (stream_.eof()) {
      out::user::log("stop");
      break;
    }

    auto it = command_map.find(cmd);
    if (it != command_map.end()) {
      auto args = read_arguments();
      if (it->second->uses_signals()) {
        server_.pause_signals().ignore_error();
        run_command(*it->second, args);
        server_.unpause_signals().ignore_error();
      } else {
        run_command(*it->second, args);
      }
    } else {
      out::error::stream("Invalid command `", cmd, "`. Use `help` for a list of commands.", out::manip::endl);
      discard_line();
    }
  }
}

void command_service::run_command(commands::base_command& cmd, const arguments_t& args) {
  cmd.run(args, server_, *this);
}

std::string command_service::read_command() {
  std::string cmd;
  stream_ >> cmd;
  return cmd;
}

arguments_t command_service::read_arguments() {
  std::string args_string;
  if (stream_.peek() == ' ') {
    stream_.get();
  }
  std::getline(stream_, args_string);
  return util::string::split(args_string, " ");
}

void command_service::discard_line() { stream_.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); }

void command_service::print_opening_line() {
  out::user::log("Use `help` for a list of commands. Use `stop` to stop the server.", out::manip::endl);
}

void command_service::stop() { server_.stop(); }

const command_map_t& command_service::get_commands() const { return command_map; }

}  // namespace input
