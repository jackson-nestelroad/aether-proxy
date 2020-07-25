/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio/ssl.hpp>

#include <aether/proxy/tcp/tls/handshake/handshake_types.hpp>
#include <aether/proxy/tcp/tls/openssl/ssl_method.hpp>
#include <aether/proxy/tcp/tls/x509/store.hpp>
#include <aether/util/bytes.hpp>
#include <aether/util/string.hpp>

namespace proxy::tcp::tls::openssl {

    /*
        Arguments needed to create an SSL context object.
        This structure manages the defaults for the various options.
    */
    struct ssl_context_args {
    private:
        static int verify_option;
        static ssl_method ssl_method_option;
        static long options_option;

    public:
        static constexpr ssl_method default_ssl_method = ssl_method::sslv23;

        int verify;
        ssl_method method;
        long options;
        std::vector<handshake::cipher_suite_name> cipher_suites;
        std::vector<std::string> alpn_protos;

        /*
            Creates an arguments object with all default options assigned.
        */
        static ssl_context_args create();

        /*
            Sets the default SSL method.
        */
        static void set_ssl_method(ssl_method method);
        
        /*
            Sets the default SSL verify option.
        */
        static void set_verify(bool verify);
    };

    std::unique_ptr<boost::asio::ssl::context> create_client_context(const ssl_context_args &args);
    void enable_hostname_verification(boost::asio::ssl::context &ctx, const std::string &sni);
}