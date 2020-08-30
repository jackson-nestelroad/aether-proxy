/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio/ssl.hpp>

#include <aether/program/options.hpp>
#include <aether/proxy/tcp/tls/handshake/handshake_types.hpp>
#include <aether/proxy/tcp/tls/openssl/ssl_method.hpp>
#include <aether/util/bytes.hpp>
#include <aether/util/string.hpp>

namespace proxy::tcp::tls::openssl {

    /*
        Arguments needed to create an SSL context object.
        This structure manages the defaults for the various options.
    */
    struct ssl_context_args {
    private:
        static long ssl_options;

    public:
        int verify;
        ssl_method method;
        long options;
        std::vector<handshake::cipher_suite_name> cipher_suites;
        std::vector<std::string> alpn_protos;
        std::string verify_file;

        /*
            Creates an arguments object with all default options assigned.
        */
        static ssl_context_args create();
    };

    std::unique_ptr<boost::asio::ssl::context> create_client_context(const ssl_context_args &args);
    void enable_hostname_verification(boost::asio::ssl::context &ctx, const std::string &sni);
}