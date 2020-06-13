/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "command_inserter.hpp"

#include <aether/input/commands/base_command.hpp>
#include <aether/input/commands/help/help.hpp>
#include <aether/input/commands/stop/stop.hpp>
#include <aether/input/commands/logs/logs.hpp>

namespace input {
    bool command_inserter::has_inserted_default = false;

    command_inserter::command_inserter() {
        if (!has_inserted_default) {
            insert_default();
        }
    }

    void command_inserter::insert_default() {
        insert_command<commands::help>();
        insert_command<commands::stop>();
        insert_command<commands::logs>();
        has_inserted_default = true;
    }
}