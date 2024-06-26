/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "command_inserter.hpp"

#include "aether/input/commands/base_command.hpp"
#include "aether/input/commands/help/help.hpp"
#include "aether/input/commands/logs/logs.hpp"
#include "aether/input/commands/stats/stats.hpp"
#include "aether/input/commands/stop/stop.hpp"

namespace input {

command_inserter::command_inserter() { insert_default(); }

void command_inserter::insert_default() {
  insert_command<commands::help>();
  insert_command<commands::stop>();
  insert_command<commands::logs>();
  insert_command<commands::stats>();
}

}  // namespace input
