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
#include <aether/proxy/tcp/tls/openssl/smart_ptrs.hpp>
#include <aether/proxy/tcp/tls/x509/certificate.hpp>
#include <aether/util/bytes.hpp>
#include <aether/util/string.hpp>

namespace proxy::tcp::tls::openssl {
    /*
        Arguments needed to create an SSL context object.
        This structure manages the defaults for the various options.
    */
    struct ssl_context_args {
    private:
        static constexpr long default_options = 
            SSL_OP_CIPHER_SERVER_PREFERENCE
            | boost::asio::ssl::context::no_compression
            | boost::asio::ssl::context::default_workarounds
            | boost::asio::ssl::context::single_dh_use;

        static constexpr long sslv23_options = 
            default_options
            | boost::asio::ssl::context::no_sslv2
            | boost::asio::ssl::context::no_sslv3;

    public:
        int verify;
        ssl_method method;
        long options;
        std::string verify_file;
        std::optional<std::function<bool(bool, boost::asio::ssl::verify_context &)>> verify_callback;
        std::vector<handshake::cipher_suite_name> cipher_suites;
        std::vector<std::string> alpn_protos;
        std::optional<SSL_CTX_alpn_select_cb_func> alpn_select_callback;
        std::optional<std::string> server_alpn;

        static long get_options_for_method(ssl_method method);
    };

    struct ssl_server_context_args {
        ssl_context_args base_args;
        const x509::certificate &cert;
        const openssl::ptrs::evp_pkey &pkey;
        const openssl::ptrs::dh &dhparams;
        std::vector<x509::certificate> cert_chain;
    };

    std::unique_ptr<boost::asio::ssl::context> create_ssl_context(ssl_context_args &args);
    void enable_hostname_verification(boost::asio::ssl::context &ctx, const std::string &sni);
}