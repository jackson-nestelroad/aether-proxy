/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#include <aether/proxy/server_components.hpp>
#include <aether/proxy/connection/connection_flow.hpp>
#include <aether/proxy/types.hpp>
#include <aether/proxy/error/exceptions.hpp>
#include <aether/util/console.hpp>

namespace proxy {
    /*
        Wrapping class for boost::asio::ip::tcp::acceptor.
        Accepts new connections.
    */
    class acceptor {
    private:
        program::options &options;
        concurrent::io_context_pool &io_contexts;
        connection::connection_manager &connection_manager;

        boost::asio::ip::tcp::endpoint endpoint;
        boost::asio::ip::tcp::acceptor acc;
        std::atomic<bool> is_stopped;

    public:
        acceptor(server_components &components);
        acceptor() = delete;
        ~acceptor() = default;
        acceptor(const acceptor &other) = delete;
        acceptor &operator=(const acceptor &other) = delete;
        acceptor(acceptor &&other) noexcept = delete;
        acceptor &operator=(acceptor &&other) noexcept = delete;

        void start();
        void stop();
        void init_accept();
        void on_accept(connection::connection_flow &connection, const boost::system::error_code &error);
        
        boost::asio::ip::tcp::endpoint get_endpoint() const;
    };
}