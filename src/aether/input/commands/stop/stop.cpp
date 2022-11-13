/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "stop.hpp"

namespace input::commands {

void stop::run(const arguments_t& args, proxy::server& server, command_service& owner) { owner.stop(); }

}  // namespace input::commands
