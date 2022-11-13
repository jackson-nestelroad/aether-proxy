/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include "aether/proxy/server.hpp"

namespace interceptors::examples::teapot {

using namespace proxy::tcp;
using namespace proxy::tcp::intercept;
using namespace proxy::connection;

// An example for inserting "418 I'm a teapot" at different endpoints.
void attach_teapot_example(proxy::server& server);

}  // namespace interceptors::examples::teapot
