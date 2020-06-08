/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <set>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <aether/proxy/connection/connection_flow.hpp>
#include <aether/proxy/connection_handler.hpp>
#include <aether/proxy/tcp/intercept/interceptor_manager.hpp>

namespace proxy::connection {
    /*
        Small class to manage ongoing connection flows.
        Owns connection flows and connection handlers to assure they are not destroyed until
            their work is finished.
    */
    class connection_manager 
        : private boost::noncopyable {
    private:
        std::set<std::shared_ptr<connection_flow>> connections;
        std::set<std::shared_ptr<connection_handler>> services;

        tcp::intercept::interceptor_manager &interceptors;

        /*
            Stops an existing service, deleting it from the records.
        */
        void stop(std::weak_ptr<connection_handler> service);

    public:
        connection_manager(tcp::intercept::interceptor_manager &interceptors);

        connection_flow &new_connection(boost::asio::io_service &ios);

        /*
            Starts managing and handling a new connection flow.
            This should be called after a client has connected.
        */
        void start(connection_flow &flow);

        /*
            Stop all connections immediately.
        */
        void stop_all();
    };
}