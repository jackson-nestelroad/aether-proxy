/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "server_builder.hpp"
#include "server.hpp"

namespace proxy {
    server server_builder::build() {
        return server(options);
    }
}