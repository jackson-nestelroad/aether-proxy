/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

#include <aether/proxy/types.hpp>
#include <aether/proxy/connection/connection_flow.hpp>
#include <aether/proxy/tcp/intercept/interceptor_manager.hpp>

namespace proxy {
    class connection_handler;

    /*
        Provides fundamental methods and data for all specialized services.
        All specialized services must inherit this class.
    */
    class base_service
        : private boost::noncopyable {
    protected:
        boost::asio::io_context &ioc;
        connection::connection_flow &flow;
        connection_handler &owner;
        tcp::intercept::interceptor_manager &interceptors;

    public:
        base_service(connection::connection_flow &flow, connection_handler &owner,
            tcp::intercept::interceptor_manager &interceptors);
        virtual ~base_service();

        /*
            Starts the service asynchronously.
            Once the service finishes, the finished callback passed to this method will run.
        */
        virtual void start() = 0;

        /*
            Stops the service or signals the end of the service.
        */
        void stop();
    };
}