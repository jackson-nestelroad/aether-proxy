/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/concurrent/io_context_pool.hpp>
#include <aether/proxy/connection/connection_manager.hpp>
#include <aether/proxy/tcp/intercept/interceptor_services.hpp>
#include <aether/proxy/tcp/tls/x509/client_store.hpp>
#include <aether/proxy/tcp/tls/x509/server_store.hpp>
#include <aether/program/options.hpp>

namespace proxy {
    /*
        Class for owning all server-wide components.
    */
    class server_components {
    private:
        server_components(const program::options &options);

        std::unique_ptr<tcp::tls::x509::client_store> client_store_ptr;
        std::unique_ptr<tcp::tls::x509::server_store> server_store_ptr;

    public:
        server_components() = delete;
        ~server_components() = default;
        server_components(const server_components &other) = delete;
        server_components &operator=(const server_components &other) = delete;
        server_components(server_components &&other) noexcept = delete;
        server_components &operator=(server_components &&other) noexcept = delete;

        program::options options;
        concurrent::io_context_pool io_contexts;
        tcp::intercept::interceptor_manager interceptors;
        connection::connection_manager connection_manager;

        inline tcp::tls::x509::client_store &client_store() {
            return *client_store_ptr;
        }

        inline tcp::tls::x509::server_store &server_store() {
            return *server_store_ptr;
        }

        friend class server;
        friend class server_builder;
    };
}