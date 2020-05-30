/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "tunnel_loop.hpp"

namespace proxy::tcp::tunnel {
    tunnel_loop::tunnel_loop(connection::base_connection &source, connection::base_connection &destination)
        : source(source),
        destination(destination)
    { }

    void tunnel_loop::start(const callback &handler) {
        on_finished = handler;
        is_finished = false;
        source.set_mode(connection::base_connection::io_mode::tunnel);
        read();
    }

    void tunnel_loop::read() {
        source.read_async(boost::bind(&tunnel_loop::on_read, this,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void tunnel_loop::on_read(const boost::system::error_code &error, std::size_t bytes_transferred) {
        if (error != boost::system::errc::success) {
            finish();
        }
        else {
            write();
        }
    }

    void tunnel_loop::write() {
        destination.output_stream() << source.input_stream().rdbuf();
        // No timeout because there is a timeout on the read operation already
        // Timeout cancels ALL socket operations
        // Attempting to use the same timeout service will cause the latter operation to never timeout
        destination.write_untimed_async(boost::bind(&tunnel_loop::on_write, this,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void tunnel_loop::on_write(const boost::system::error_code &error, std::size_t bytes_transferred) {
        if (error != boost::system::errc::success) {
            finish();
        }
        else {
            read();
        }
    }

    void tunnel_loop::finish() {
        source.set_mode(connection::base_connection::io_mode::regular);
        is_finished = true;
        on_finished();
    }

    bool tunnel_loop::finished() const {
        return is_finished;
    }
}