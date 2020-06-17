/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "acceptor.hpp"

namespace proxy {
    acceptor::acceptor(const program::options &opts, concurrent::io_service_pool &io_services,
        connection::connection_manager &connection_manager)
        : io_services(io_services),
        endpoint(opts.ip_v6 ? boost::asio::ip::tcp::v6() : boost::asio::ip::tcp::v4(), opts.port),
        acc(io_services.get_io_service(), endpoint),
        is_stopped(false),
        connection_manager(connection_manager)
    {
        connection::base_connection::set_timeout_duration(opts.timeout);
        connection::base_connection::set_tunnel_timeout_duration(opts.tunnel_timeout);
        tcp::http::http1::http_parser::set_body_size_limit(opts.body_size_limit);

        boost::system::error_code ec;
        if (opts.ip_v6) {
            acc.set_option(boost::asio::ip::v6_only(false), ec);
            if (ec != boost::system::errc::success) {
                throw error::ipv6_exception(out::string::stream(
                    "Could not configure dual stack socket (error code = ",
                    ec.value(),
                    "). Use --ipv6=false to disable IPv6."
                ));
            }
        }
        acc.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
        if (ec != boost::system::errc::success) {
            throw error::acceptor_exception("Could not configure socket option SO_REUSEADDR.");
        }
    }

    void acceptor::start() {
        acc.listen();
        init_accept();
    }

    void acceptor::stop() {
        is_stopped.store(true);
    }

    void acceptor::init_accept() {
        // auto new_connection = connection::connection_flow::create(io_services.get_io_service());
        auto &new_connection = connection_manager.new_connection(io_services.get_io_service());

        acc.async_accept(new_connection.client.get_socket(),
            boost::bind(&acceptor::on_accept, this, std::ref(new_connection), boost::asio::placeholders::error));
    }

    void acceptor::on_accept(connection::connection_flow &connection, const boost::system::error_code &error) {
        if (error != boost::system::errc::success) {
            init_accept();
            throw error::acceptor_exception(out::string::stream(error.message(), " (", error, ')'));
        }

        // out::console::log("Connection!", connection->client.get_socket().native_handle());
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