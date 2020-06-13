/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <string>

#include <aether/proxy/types.hpp>

namespace program {
    /*
        All command-line options for the proxy server.
        Attached to the options_parser class in functions.cpp.
    */
    struct options {
        proxy::port_t port;
        bool help;
        bool ip_v6;
        int thread_pool_size;
        std::size_t timeout;
        std::size_t tunnel_timeout;
        std::size_t body_size_limit;
        bool run_command_service;
        bool run_logs;
        bool run_silent;
    };
}