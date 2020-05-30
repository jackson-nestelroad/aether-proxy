/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>

#include <aether/proxy/connection/connection_flow.hpp>
#include <aether/proxy/base_service.hpp>
#include <aether/proxy/tcp/http/http1/http_service.hpp>

namespace proxy {
    /*
        Handles a connection by passing it to a service.
    */
    class connection_handler
        : public std::enable_shared_from_this<connection_handler>,
        private boost::noncopyable {
    private:
        io_service::ptr ios;
        callback on_finished;

        // The single connection flow being managed
        connection::connection_flow::ptr flow;

        // The current service handling the connection flow
        base_service::ptr http_service;

        connection_handler(connection::connection_flow::ptr flow);

    public:
        using ptr = std::shared_ptr<connection_handler>;
        using weak_ptr = std::weak_ptr<connection_handler>;
        static ptr create(connection::connection_flow::ptr flow);

        /*
            Starts handling the connection by routing it to specialized services.
            Once the connection finishes and is disconnected, the callback passed here will be called.
        */
        void start(const callback &handler);

        /*
            Immediately switches the flow to a new service to be handled.
        */
        template <typename T>
        std::enable_if_t<std::is_base_of_v<base_service, T>, void> switch_service() {
            http_service.reset(new T(flow, *this));
            http_service->start();
        }

        /*
            Stops handling the connection by disconnecting from the client and server.
            Calls the finished callback passed when the handler was started.
        */
        void stop();
    };
}