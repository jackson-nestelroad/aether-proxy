/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "options.hpp"
#include <aether/proxy/server.hpp>

namespace program {
    /*
        Prints the help message to out::console.
    */
    void options::print_help() const {
        out::console::log("Aether is a simple HTTP/HTTPS proxy server written in C++ using Boost.Asio and OpenSSL.");
        out::console::log("Usage:", command_name, usage);
        out::console::log();
        parser.print_options();
        out::console::log();
    }

    void options::add_options() {
        parser.add_option<proxy::port_t>("port", 'p', &port, 3000, "Specifies the port to listen on.", { }, { });

        parser.add_option<bool>("help", &help, false, "Displays help and options.", { }, { });

        parser.add_option<bool>("ipv6", &ipv6, true, "Enables IPv6 using a dual stack socket.", { }, { });

        parser.add_option<int>("threads", &thread_pool_size,
            util::validate::resolve_default_value<int>([](auto t) { return t > 0; }, std::thread::hardware_concurrency() * 2, 2),
            "Number of threads for the server to run.",
            [](auto t) { return t > 0; }, { });

        parser.add_option<std::size_t, proxy::milliseconds>("timeout", &timeout, 500000,
            "Milliseconds for connect, read, and write operations to timeout.",
            [](auto t) { return t != 0; }, [](auto t) { return proxy::milliseconds(t); });

        parser.add_option<std::size_t, proxy::milliseconds>("tunnel-timeout", &tunnel_timeout, 30000,
            "Milliseconds for tunnel operations to timeout.",
            [](auto t) { return t != 0; }, [](auto t) { return proxy::milliseconds(t); });

        parser.add_option<std::size_t>("body-size-limit", &body_size_limit, 2'000'000, // 2 MB
            "Maximum body size (in bytes) to allow through the proxy. Must be greater than 4096.",
            [](auto l) { return l > 4096; }, { });

        parser.add_option<std::string, boost::asio::ssl::context::method>("ssl-method", &ssl_method, boost::lexical_cast<std::string>(boost::asio::ssl::context::method::sslv23),
            "SSL method to be used by the server when connecting to an upstream server.",
            &util::validate::lexical_castable<std::string, boost::asio::ssl::context::method>, [](auto m) { return boost::lexical_cast<boost::asio::ssl::context::method>(m); });

        parser.add_option<bool, int>("ssl-verify", &ssl_verify, true,
            "Verify the upstream server's SSL certificate.",
            { }, [](bool verify) { return verify ? boost::asio::ssl::context::verify_peer : boost::asio::ssl::context::verify_none; });

        parser.add_option<std::string>("upstream-trusted-ca-file", &ssl_verify_upstream_trusted_ca_file_path,
            proxy::tcp::tls::x509::store::default_trusted_certificates_file.string(),
            "Path to a PEM-formatted trusted CA certificate for upstream verification.",
            [](const std::string &path) { return std::ifstream(path).good(); }, { });

        parser.add_option<bool>("negotiate-ciphers", &ssl_negotiate_ciphers, false,
            "Negotiate the SSL cipher suites with the server, regardless of the options the client sends.",
            { }, { });

        parser.add_option<bool>("negotiate-alpn", &ssl_negotiate_alpn, false,
            "Negotiate the ALPN protocol with the server, regardless of the options the client sends.",
            { }, { });


        parser.add_option<bool>("commands", &run_command_service, true,
            "Runs a command-line service for interacting with the server in real time.",
            { }, { });
        parser.add_option<bool>("logs", &run_logs, false,
            "Logs all server activity to stdout.",
            { }, { });
        parser.add_option<bool>("silent", 's', &run_silent, false,
            "Prints nothing to stdout while the server runs.",
            { }, { });
    }

    void options::parse_cmdline(int argc, char *argv[]) {
        std::call_once(options_added, boost::bind(&options::add_options, this));
        command_name = argv[0];
        usage = "[OPTIONS]";

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
        if (help) {
            print_help();
            exit(0);
        }
    }

    void options::parse_cmdline_options(int argc, char *argv[]) {
        raw_instance().parse_cmdline(argc, argv);
    }
}