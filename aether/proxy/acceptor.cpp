/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "acceptor.hpp"

namespace proxy {
    acceptor::acceptor(server_components &components)
        : options(components.options),
        io_contexts(components.io_contexts),
        connection_manager(components.connection_manager),
        endpoint(options.ipv6 ? boost::asio::ip::tcp::v6() : boost::asio::ip::tcp::v4(), options.port),
        acc(io_contexts.get_io_context(), endpoint),
        is_stopped(false)
    {
        boost::system::error_code ec;
        if (options.ipv6) {
            acc.set_option(boost::asio::ip::v6_only(false), ec);
            acc.set_option(boost::asio::socket_base::send_buffer_size(64 * 1024));
            if (ec != boost::system::errc::success) {
                throw error::ipv6_error_exception(out::string::stream(
                    "Could not configure dual stack socket (error code = ",
                    ec.value(),
                    "). Use --ipv6=false to disable IPv6."
                ));
            }
        }
        acc.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
        if (ec != boost::system::errc::success) {
            throw error::acceptor_error_exception("Could not configure socket option SO_REUSEADDR.");
        }
    }

    void acceptor::start() {
        acc.listen(options.connection_queue_limit);
        init_accept();
    }

    void acceptor::stop() {
        is_stopped.store(true);
    }

    void acceptor::init_accept() {
        auto &new_connection = connection_manager.new_connection(io_contexts.get_io_context());

        acc.async_accept(new_connection.client.get_socket(),
            boost::bind(&acceptor::on_accept, this, std::ref(new_connection), boost::asio::placeholders::error));
    }

    void acceptor::on_accept(connection::connection_flow &connection, const boost::system::error_code &error) {
        if (error != boost::system::errc::success) {
            connection_manager.destroy(connection);
            init_accept();
            throw error::acceptor_error_exception(out::string::stream(error.message(), " (", error, ')'));
        }

        connection_manager.start(connection);

        if (!is_stopped.load()) {
            init_accept();
        }
        else {
            acc.close();
        }
    }

    boost::asio::ip::tcp::endpoint acceptor::get_endpoint() const {
        return endpoint;
    }
}