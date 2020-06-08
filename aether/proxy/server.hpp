/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <string>
#include <memory>
#include <thread>
#include <boost/asio.hpp>

#include <aether/proxy/acceptor.hpp>
#include <aether/proxy/types.hpp>
#include <aether/proxy/concurrent/io_service_pool.hpp>
#include <aether/proxy/connection/connection_manager.hpp>
#include <aether/proxy/tcp/intercept/interceptor_manager.hpp>
#include <aether/program/options.hpp>

namespace proxy {
    /*
        The server class used to startup all of the boost::asio:: services.
        Manages the acceptor port and io_services pool.
    */
    class server 
        : private boost::noncopyable {
    private:
        program::options options;
        std::unique_ptr<acceptor> acc;
        bool is_running;

        // Dependency injection services

        concurrent::io_service_pool io_services;
        connection::connection_manager connection_manager;

        /*
            Method for calling io_service.run() to start the boost::asio:: services.
        */
        static void run_service(boost::asio::io_service &ios);

    public:
        // Public dependency injection services

        tcp::intercept::interceptor_manager interceptors;
        
        server(const program::options &options);
        ~server();

        void start();
        void stop();

        std::string endpoint_string() const;
    };
}