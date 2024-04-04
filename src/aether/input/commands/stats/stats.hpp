/*********************************************

  Copyright (c) Jackson Nestelroad 2024
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <memory>
#include <string>

#include "aether/input/commands/base_command.hpp"

namespace input::commands {

class stats : public base_command {
 public:
  void run(const arguments_t& args, proxy::server& server, command_service& caller) override;

  inline std::string name() const override { return "stats"; }
  inline std::string args() const override { return "(watch)"; }
  inline std::string description() const override { return "Displays stats for the server."; }
  inline bool uses_signals() const override { return true; }

 private:
  void print_stats(proxy::server& server);
};

}  // namespace input::commands
