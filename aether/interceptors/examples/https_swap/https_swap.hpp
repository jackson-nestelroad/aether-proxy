/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/server.hpp>

using namespace proxy::tcp;
using namespace proxy::tcp::intercept;
using namespace proxy::connection;

namespace interceptors::examples {
    /*
        An example for swapping one HTTPS site for another.
    */
    void attach_https_swap_example(proxy::server &server);
}
