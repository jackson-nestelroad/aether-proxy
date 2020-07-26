/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <string>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <aether/proxy/types.hpp>
#include <aether/util/singleton.hpp>

namespace program {
    /*
        All command-line options for the proxy server.
        Attached to the options_parser class in functions.cpp.
    */
    struct options 
        : public util::singleton<options> {
        proxy::port_t port;
        bool help;
        bool ipv6;
        int thread_pool_size;
        proxy::milliseconds timeout { 0 };
        proxy::milliseconds tunnel_timeout { 0 };
        std::size_t body_size_limit;
        
        boost::asio::ssl::context::method ssl_method;
        int ssl_verify;
        std::string ssl_verify_upstream_trusted_ca_file_path;
        bool ssl_negotiate_ciphers;
        bool ssl_negotiate_alpn;

        bool run_command_service;
        bool run_logs;
        bool run_silent;
    };
}