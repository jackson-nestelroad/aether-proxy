/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/util/buffer_segment.hpp>
#include <aether/proxy/connection/base_connection.hpp>

namespace proxy::tcp::tunnel {
    /*
        Implements an asynchronous read/write loop from one connection to another.
        Connections must out-live any tunnel loop it is connected to.
    */
    class tunnel_loop {
    private:
        connection::base_connection &source;
        connection::base_connection &destination;
        callback on_finished;
        bool is_finished;

        void read();
        void on_read(const boost::system::error_code &error, std::size_t bytes_transferred);
        void write();
        void on_write(const boost::system::error_code &error, std::size_t bytes_transferred);
        void finish();

    public:
        tunnel_loop(connection::base_connection &source, connection::base_connection &destination);
        void start(const callback &handler);
        bool finished() const;
    };
}