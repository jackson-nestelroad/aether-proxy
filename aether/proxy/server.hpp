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
        concurrent::io_service_pool io_services;
        std::unique_ptr<acceptor> acc;
        bool is_running;

        /*
            Method for calling io_service.run() to start the boost::asio:: services.
        */
        static void run_service(io_service::ptr ios);

    public:
        server(const program::options &options);
        ~server();

        void start();
        void stop();

        std::string endpoint_string() const;
    };
}