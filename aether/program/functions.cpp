/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "functions.hpp"

namespace program {
    /*
        Prints the help message to out::console.
    */
    void help(const std::string &usage, const options_parser &parser) {
        out::console::log("Aether is a simple HTTP proxy server written in C++ using the Boost.Asio library.");
        out::console::log("Usage:", usage);
        out::console::log();
        parser.print_options();
        out::console::log();
    }

    options parse_cmdline_options(int argc, char *argv[]) {
        std::string usage = argv[0] + std::string(" [OPTIONS]");
        options opts;
        options_parser parser;

        // Add all options to the parser
        parser.add_option<proxy::port_t>("port", 'p', &opts.port, 3000, "Specifies the port to listen on.", { });
        
        parser.add_option<bool>("help", &opts.help, false, "Displays help and options.", { });
        
        parser.add_option<bool>("ipv6", &opts.ip_v6, true, "Enables IPv6 using a dual stack socket.", { });
        parser.add_option<int>("threads", &opts.thread_pool_size, 
            util::validate::resolve_default_value<int>([](auto t) { return t > 0; }, std::thread::hardware_concurrency() * 2, 2),
            "Number of threads for the server to run.",
            [](auto t) { return t > 0; });
        
        parser.add_option<std::size_t>("timeout", &opts.timeout, proxy::connection::base_connection::default_timeout_ms, 
            "Milliseconds for connect, read, and write operations to timeout.", 
            [](auto t) { return t != 0; });
        parser.add_option<std::size_t>("tunnel-timeout", &opts.tunnel_timeout, proxy::connection::base_connection::default_tunnel_timeout_ms,
            "Milliseconds for tunnel operations to timeout.",
            [](auto t) { return t != 0; });
        
        parser.add_option<std::size_t>("body-size-limit", &opts.body_size_limit, proxy::tcp::http::http1::http_parser::default_body_size_limit,
            "Maximum body size (in bytes) to allow through the proxy. Must be greater than 4096.",
            [](auto l) { return l > 4096; });
        
        parser.add_option<std::string>("ssl-method", &opts.ssl_method, boost::lexical_cast<std::string>(proxy::tcp::tls::openssl::ssl_context_args::default_ssl_method),
            "SSL method to be used by the server when connecting to an upstream server.",
            &util::validate::lexical_castable<std::string, proxy::tcp::tls::openssl::ssl_method>);
        parser.add_option<bool>("ssl-verify", &opts.ssl_verify, true,
            "Verify the upstream server's SSL certificate.",
            { });
        parser.add_option<std::string>("upstream-trusted-ca-file", &opts.ssl_verify_upstream_trusted_ca_file_path, 
            proxy::tcp::tls::x509::store::default_trusted_certificates_file.string(),
            "Path to a PEM-formatted trusted CA certificate for upstream verification.",
            [](const std::string &path) { return std::ifstream(path).good(); });


        parser.add_option<bool>("commands", &opts.run_command_service, true,
            "Runs a command-line service for interacting with the server in real time.",
            { });
        parser.add_option<bool>("logs", &opts.run_logs, false,
            "Logs all server activity to stdout.",
            { });
        parser.add_option<bool>("silent", 's', &opts.run_silent, false,
            "Prints nothing to stdout while the server runs.",
            { });

        try {
            parser.parse(argc, argv);
        }
        catch (const option_exception &ex) {
            out::error::log(ex.what());
            out::error::log("Usage: ", usage);
            out::error::stream("Use `", argv[0], " --help` to view options", out::manip::endl);
            exit(1);
        }

        // The help option ends the program
        if (opts.help) {
            program::help(usage, parser);
            exit(0);
        }

        return opts;
    }
}