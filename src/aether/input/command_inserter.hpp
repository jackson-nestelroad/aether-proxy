/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <memory>
#include <type_traits>

#include "aether/input/command_service.hpp"
#include "aether/input/commands/base_command.hpp"
#include "aether/input/types.hpp"

namespace input {

// Static class for inserting commands into command_service's static command map.
// Commands are inserted ONCE by the class' constructor.
class command_inserter {
 public:
  // Inserts a new command into all command service instances.
  template <typename T>
  static std::enable_if_t<std::is_base_of_v<commands::base_command, T>, void> insert_command() {
    auto new_command = std::make_shared<T>();
    command_service::command_map.emplace(new_command->name(),
                                         std::static_pointer_cast<commands::base_command>(new_command));
  }

 private:
  static bool has_inserted_default;

  command_inserter();

  static void insert_default();

  friend class command_service;
};
}  // namespace input
