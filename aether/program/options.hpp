/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <string>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <aether/proxy/types.hpp>
#include <aether/util/console.hpp>
#include <aether/util/validate.hpp>
#include <aether/program/options_parser.hpp>

namespace program {
    /*
        All command-line options for the proxy server.
        Accessable anywhere in the application.
    */
    struct options {
    private:
        options_parser parser;
        bool options_added = false;

        std::string command_name;
        std::string usage;

        // Add all options to the parser
        void add_options();

        /*
            Print help information for this command to the command-line.
        */
        void print_help() const;

    public:
        options() = default;
        ~options() = default;
        options(const options &other) = default;
        options &operator=(const options &other) = default;
        options(options &&other) noexcept = default;
        options &operator=(options &&other) noexcept = default;

        proxy::port_t port;
        bool help;
        bool ipv6;
        int thread_pool_size;
        int connection_queue_limit;
        proxy::milliseconds timeout { 0 };
        proxy::milliseconds tunnel_timeout { 0 };
        std::size_t body_size_limit;

        bool ssl_passthrough;
        bool ssl_passthrough_strict;
        boost::asio::ssl::context::method ssl_client_method;
        boost::asio::ssl::context::method ssl_server_method;
        int ssl_verify;
        bool ssl_negotiate_ciphers;
        bool ssl_negotiate_alpn;
        bool ssl_supply_server_chain_to_client;

        std::string ssl_cert_store_properties;
        std::string ssl_cert_store_dir;
        std::string ssl_dhparam_file;
        std::string ssl_verify_upstream_trusted_ca_file_path;
        
        bool websocket_passthrough;
        bool websocket_passthrough_strict;
        bool websocket_intercept_messages_by_default;

        bool run_interactive;
        bool run_logs;
        bool run_silent;

        std::string log_file_name;

        /*
            Parses all command-line options according to the internal configuration of the instance.
        */
        void parse_cmdline(int argc, char *argv[]);
    };
}