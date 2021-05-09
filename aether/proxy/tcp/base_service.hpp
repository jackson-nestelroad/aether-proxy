/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <array>
#include <boost/asio.hpp>

#include <aether/proxy/types.hpp>
#include <aether/proxy/connection/connection_flow.hpp>
#include <aether/proxy/tcp/intercept/interceptor_services.hpp>

namespace proxy {
    class server_components;
    class connection_handler;
}

namespace proxy::tcp {

    /*
        Provides fundamental methods and data for all specialized services.
        All specialized services must inherit this class.
    */
    class base_service {
    private:
        static constexpr std::array<std::string_view, 3> forbidden_hosts = {
            "localhost",
            "127.0.0.1",
            "::1"
        };

        void on_connect_server(const boost::system::error_code &error, const err_callback &handler);

    protected:
        boost::asio::io_context &ioc;
        program::options &options;
        connection::connection_flow &flow;
        connection_handler &owner;
        tcp::intercept::interceptor_manager &interceptors;

        /*
            Sets the server to connect to later.
        */
        void set_server(std::string_view host, port_t port);

        /*
            Connects to the server asynchronously.
        */
        void connect_server_async(const err_callback &handler);

    public:
        base_service(connection::connection_flow &flow, connection_handler &owner, server_components &components);
        base_service() = delete;
        virtual ~base_service();
        base_service(const base_service &other) = delete;
        base_service &operator=(const base_service &other) = delete;
        base_service(base_service &&other) noexcept = delete;
        base_service &operator=(base_service &&other) noexcept = delete;

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