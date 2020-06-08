/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "client_connection.hpp"

namespace proxy::connection {
    client_connection::client_connection(boost::asio::io_service &ios)
        : base_connection(ios)
    { }
}