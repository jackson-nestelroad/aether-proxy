/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <string>

#include "aether/input/commands/base_command.hpp"

namespace input::commands {

// Command that stops the command service, which will stop the server afterwards.
class stop : public base_command {
 public:
  void run(const arguments_t& args, proxy::server& server, command_service& caller) override;

  inline std::string name() const override { return "stop"; }
  inline std::string args() const override { return ""; }
  inline std::string description() const override { return "Stops the server."; }
  inline bool uses_signals() const override { return false; }
};

}  // namespace input::commands
