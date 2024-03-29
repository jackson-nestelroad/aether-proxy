/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include "aether/proxy/server.hpp"

// Functions for attaching interceptors to a server.

namespace interceptors {

// Attaches the default interceptors.
void attach_default(proxy::server& server);

}  // namespace interceptors
