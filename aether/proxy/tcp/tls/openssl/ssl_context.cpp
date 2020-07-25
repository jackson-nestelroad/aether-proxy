/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "ssl_context.hpp"

namespace proxy::tcp::tls::openssl {
    int ssl_context_args::verify_option = boost::asio::ssl::context::verify_peer;
    ssl_method ssl_context_args::ssl_method_option = ssl_method::sslv23;
    long ssl_context_args::options_option =
        SSL_OP_CIPHER_SERVER_PREFERENCE
        | boost::asio::ssl::context::no_compression
        | boost::asio::ssl::context::default_workarounds
        | boost::asio::ssl::context::no_sslv2
        | boost::asio::ssl::context::no_sslv3
        | boost::asio::ssl::context::single_dh_use;

    ssl_context_args ssl_context_args::create() {
        return {
             verify_option,
             ssl_method_option,
             options_option,
        };
    }

    void ssl_context_args::set_ssl_method(ssl_method method) {
        ssl_method_option = method;
    }

    void ssl_context_args::set_verify(bool verify) {
        verify_option = verify ? boost::asio::ssl::context_base::verify_peer : boost::asio::ssl::context_base::verify_none;
    }

    std::string to_sni(const std::string &host, port_t port) {
        return host + ':' + boost::lexical_cast<std::string>(port);
    }

    std::unique_ptr<boost::asio::ssl::context> create_client_context(const ssl_context_args &args) {
        auto ctx = std::make_unique<boost::asio::ssl::context>(args.method);
        ctx->set_verify_mode(args.verify);
        ctx->set_options(args.options);

        X509_STORE *store = x509::store::get_singleton().native_handle();
        SSL_CTX_set_cert_store(ctx->native_handle(), store);

        int res;

        res = SSL_CTX_set_cipher_list(ctx->native_handle(), util::string::join(args.cipher_suites, ":").c_str());
        if (res != 0) {
            throw error::tls::invalid_cipher_suite_list_exception { };
        }

        auto wire_alpn = util::bytes::to_wire_format<1>(args.alpn_protos);
        res = SSL_CTX_set_alpn_protos(ctx->native_handle(), wire_alpn.data(), static_cast<unsigned int>(wire_alpn.size()));
        if (res != 0) {
            throw error::tls::invalid_alpn_protos_list_exception { };
        }

        return ctx;
    }

    void enable_hostname_verification(boost::asio::ssl::context &ctx, const std::string &sni) {
        // Manutally enable hostname verification
        int res = 0;
        X509_VERIFY_PARAM *param = X509_VERIFY_PARAM_new();
        res += X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_CRL_CHECK);
        X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS | X509_CHECK_FLAG_NEVER_CHECK_SUBJECT);
        res += X509_VERIFY_PARAM_set1_host(param, sni.c_str(), sni.size());
        res += SSL_CTX_set1_param(ctx.native_handle(), param);
        X509_VERIFY_PARAM_free(param);

        if (res != 3) {
            throw error::tls::ssl_context_exception { "Failed to enable hostname verification" };
        }
    }
}