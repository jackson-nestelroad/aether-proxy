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

namespace proxy::connection {
    /*
        Small class to manage ongoing connection_flows.
        Holds service_routers and connection_flows to assure they are not destroyed when calling
            asynchronous functions.
    */
    class connection_manager 
        : private boost::noncopyable {
    private:
        std::set<connection_handler::ptr> services;

        /*
            Stops an existing service, deleting it from the records.
        */
        void stop(connection_handler::weak_ptr http_service);

    public:
        connection_manager() = default;

        /*
            Starts managing and handling a new connection flow.
            This should be called after a client has connected.
        */
        void start(connection_flow::ptr flow);

        /*
            Stop all connections immediately.
        */
        void stop_all();
    };
}