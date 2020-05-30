/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "client_connection.hpp"

namespace proxy::connection {
    client_connection::client_connection(io_service::ptr ios)
        : base_connection(ios)
    { }
}