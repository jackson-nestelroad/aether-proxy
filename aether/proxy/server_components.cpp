/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "server_components.hpp"

namespace proxy {
    server_components::server_components(const program::options &options)
        : options(options),
        io_contexts(options.thread_pool_size),
        interceptors(),
        connection_manager(*this)
    { 
        if (!options.ssl_passthrough_strict) {
            client_store_ptr = std::make_unique<tcp::tls::x509::client_store>(*this);
            server_store_ptr = std::make_unique<tcp::tls::x509::server_store>(*this);
        }
    }
}