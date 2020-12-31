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
#include <aether/proxy/concurrent/io_context_pool.hpp>
#include <aether/proxy/connection/connection_manager.hpp>
#include <aether/proxy/tcp/intercept/interceptor_services.hpp>
#include <aether/program/options.hpp>
#include <aether/util/signal_handler.hpp>
#include <aether/util/thread_blocker.hpp>

namespace proxy {
    /*
        The server class used to startup all of the boost::asio:: services.
        Manages the acceptor port and io_context pool.
    */
    class server 
        : private boost::noncopyable {
    private:
        bool is_running;
        bool needs_cleanup;

        // Dependency injection services
        // Make sure io_contexts are destroyed LAST because other objects use them

        concurrent::io_context_pool io_contexts;
        connection::connection_manager connection_manager;
        out::logging_manager log_manager;

        std::unique_ptr<acceptor> acc;
        std::unique_ptr<util::signal_handler> signals;
        util::thread_blocker blocker;

        /*
            Method for calling io_context.run() to start the boost::asio:: services.
        */
        static void run_service(boost::asio::io_context &ioc);

        void signal_stop();
        void cleanup();

    public:
        // Public dependency injection services

        tcp::intercept::interceptor_manager interceptors;
        
        server();
        ~server();

        void start();
        void stop();
        void pause_signals();
        void unpause_signals();
        void enable_logs();
        void disable_logs();

        /*
            Blocks the thread until the server is stopped internally.
            The server can stop using the stop() function or using exit signals
                registered on the signal_handler.
        */
        void await_stop();

        bool running() const;
        std::string endpoint_string() const;
        boost::asio::io_context &get_io_context();
    };
}