/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "help.hpp"

#include <string_view>

#include "aether/util/console.hpp"

namespace input::commands {

void help::run(const arguments_t& args, proxy::server& server, command_service& owner) {
  std::string_view lookup = args[0];
  const auto& commands = owner.get_commands();
  if (lookup.empty()) {
    for (const auto& [name, command] : commands) {
      out::user::log(command->name(), command->args());
      out::user::log(command->description(), out::manip::endl);
    }
  } else {
    auto it = commands.find(lookup);
    if (it != commands.end()) {
      auto& command = it->second;
      out::user::log(command->name(), command->args());
      out::user::log(command->description(), out::manip::endl);
    } else {
      out::user::stream("Could not find command `", lookup, "`. Use `help` for a list of commands.", out::manip::endl);
    }
  }
}

}  // namespace input::commands
