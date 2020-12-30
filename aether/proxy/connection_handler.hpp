/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>

#include <aether/proxy/connection/connection_flow.hpp>
#include <aether/proxy/tcp/base_service.hpp>
#include <aether/proxy/tcp/http/http1/http_service.hpp>
#include <aether/proxy/tcp/intercept/interceptor_manager.hpp>

namespace proxy {
    /*
        Handles a connection by passing it to a service.
    */
    class connection_handler
        : public std::enable_shared_from_this<connection_handler>,
        private boost::noncopyable {
    private:
        boost::asio::io_context &ioc;
        callback on_finished;

        // The single connection flow being managed
        connection::connection_flow &flow;

        // The current service handling the connection flow
        std::unique_ptr<tcp::base_service> current_service;

        tcp::intercept::interceptor_manager &interceptors;

    public:
        connection_handler(connection::connection_flow &flow, tcp::intercept::interceptor_manager &interceptors);

        /*
            Starts handling the connection by routing it to specialized services.
            Once the connection finishes and is disconnected, the callback passed here will be called.
        */
        void start(const callback &handler);

        /*
            Immediately switches the flow to a new service to be handled.
        */
        template <typename T, typename... Args>
        std::enable_if_t<std::is_base_of_v<tcp::base_service, T>, void> switch_service(Args &... args) {
            current_service = std::make_unique<T>(flow, *this, interceptors, args...);
            current_service->start();
        }

        /*
            Stops handling the connection by disconnecting from the client and server.
            Calls the finished callback passed when the handler was started.
        */
        void stop();

        connection::connection_flow &get_connection_flow() const;
    };
}