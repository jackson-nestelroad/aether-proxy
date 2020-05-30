/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#include <aether/proxy/types.hpp>
#include <aether/proxy/connection/connection_manager.hpp>
#include <aether/proxy/connection/connection_flow.hpp>
#include <aether/proxy/concurrent/io_service_pool.hpp>
#include <aether/proxy/error/exceptions.hpp>
#include <aether/program/options.hpp>
#include <aether/util/console.hpp>

namespace proxy {
    /*
        Wrapping class for boost::asio::ip::tcp::acceptor.
        Accepts new connections.
    */
    class acceptor 
        : private boost::noncopyable {
    private:
        concurrent::io_service_pool &io_services;
        boost::asio::ip::tcp::endpoint endpoint;
        boost::asio::ip::tcp::acceptor acc;
        std::atomic<bool> is_stopped;
        connection::connection_manager connection_manager;

    public:
        acceptor(concurrent::io_service_pool &io_services, const program::options &options);

        void start();
        void stop();
        void init_accept();
        void on_accept(connection::connection_flow::ptr connection, const boost::system::error_code &error);
        
        boost::asio::ip::tcp::endpoint get_endpoint() const;
    };
}