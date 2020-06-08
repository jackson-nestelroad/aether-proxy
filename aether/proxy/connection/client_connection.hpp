/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>

#include <aether/proxy/connection/base_connection.hpp>

namespace proxy::connection {
    /*
        A connection to the client (whoever initiated the request).
    */
    class client_connection
        : public base_connection {
    private:

    public:
        client_connection(boost::asio::io_service &ios);
    };
}