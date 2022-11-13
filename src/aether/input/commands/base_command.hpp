/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <string>

#include "aether/input/command_service.hpp"
#include "aether/input/types.hpp"
#include "aether/proxy/server.hpp"

namespace input::commands {

// Interface to be implemented by all commands.
class base_command {
 public:
  virtual void run(const arguments_t& args, proxy::server& server, command_service& caller) = 0;
  virtual std::string name() const = 0;
  virtual std::string args() const = 0;
  virtual std::string description() const = 0;
  virtual bool uses_signals() const;

  virtual ~base_command() = default;
};

}  // namespace input::commands
