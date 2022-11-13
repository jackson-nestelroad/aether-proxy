/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/server.hpp>

namespace interceptors::examples::https_swap {

using namespace proxy::tcp;
using namespace proxy::tcp::intercept;
using namespace proxy::connection;

// An example for swapping one HTTPS site for another.
void attach_https_swap_example(proxy::server& server);

}  // namespace interceptors::examples::https_swap
