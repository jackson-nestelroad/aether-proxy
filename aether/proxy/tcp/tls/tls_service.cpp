/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "tls_service.hpp"
#include <aether/proxy/connection_handler.hpp>

namespace proxy::tcp::tls {
    std::unique_ptr<x509::client_store> tls_service::client_store;
    std::unique_ptr<x509::server_store> tls_service::server_store;

    // See https://ssl-config.mozilla.org/#config=old
    const std::vector<handshake::cipher_suite_name> tls_service::default_client_ciphers {
        handshake::cipher_suite_name::ECDHE_ECDSA_AES128_GCM_SHA256,
        handshake::cipher_suite_name::ECDHE_RSA_AES128_GCM_SHA256,
        handshake::cipher_suite_name::ECDHE_ECDSA_AES256_GCM_SHA384,
        handshake::cipher_suite_name::ECDHE_RSA_AES256_GCM_SHA384,
        handshake::cipher_suite_name::ECDHE_ECDSA_CHACHA20_POLY1305_OLD,
        handshake::cipher_suite_name::ECDHE_RSA_CHACHA20_POLY1305_OLD,
        handshake::cipher_suite_name::DHE_RSA_AES128_GCM_SHA256,
        handshake::cipher_suite_name::DHE_RSA_AES256_GCM_SHA384,
        handshake::cipher_suite_name::DHE_RSA_CHACHA20_POLY1305_OLD,
        handshake::cipher_suite_name::ECDHE_ECDSA_AES128_SHA256,
        handshake::cipher_suite_name::ECDHE_RSA_AES128_SHA256,
        handshake::cipher_suite_name::ECDHE_ECDSA_AES128_SHA,
        handshake::cipher_suite_name::ECDHE_RSA_AES128_SHA,
        handshake::cipher_suite_name::ECDHE_ECDSA_AES256_SHA384,
        handshake::cipher_suite_name::ECDHE_RSA_AES256_SHA384,
        handshake::cipher_suite_name::ECDHE_ECDSA_AES256_SHA,
        handshake::cipher_suite_name::ECDHE_RSA_AES256_SHA,
        handshake::cipher_suite_name::DHE_RSA_AES128_SHA256,
        handshake::cipher_suite_name::DHE_RSA_AES256_SHA256,
        handshake::cipher_suite_name::AES128_GCM_SHA256,
        handshake::cipher_suite_name::AES256_GCM_SHA384,
        handshake::cipher_suite_name::AES128_SHA256,
        handshake::cipher_suite_name::AES256_SHA256,
        handshake::cipher_suite_name::AES128_SHA,
        handshake::cipher_suite_name::AES256_SHA,
        handshake::cipher_suite_name::DES_CBC3_SHA
    };

    tls_service::tls_service(connection::connection_flow &flow, connection_handler &owner,
        tcp::intercept::interceptor_manager &interceptors)
        : base_service(flow, owner, interceptors)
    { }
    
    void tls_service::start() {
        read_client_hello();
    }

    void tls_service::read_client_hello() {
        flow.client.read_async(boost::bind(&tls_service::on_read_client_hello, this,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void tls_service::on_read_client_hello(const boost::system::error_code &error, std::size_t bytes_transferred) {
        if (error != boost::system::errc::success) {
            // There may be data in the stream that is still inteded for the server
            // Switch to a TCP tunnel to be safe
            handle_not_client_hello();
        }
        else {
            try {
                auto buf = flow.client.const_input_buffer();
                std::size_t remaining = client_hello_reader.read(buf, bytes_transferred);

                // Parsing complete
                if (remaining == 0) {
                    handle_client_hello();
                }
                // Need to read again (unlikely)
                else {
                    read_client_hello();
                }
            }
            // The data was not formatted as we would expect, but it still
            // may be intended for the server
            catch (const error::tls::invalid_client_hello_exception &) {
                handle_not_client_hello();
            }
        }
    }

    void tls_service::handle_not_client_hello() {
        // This is NOT a Client Hello message, so TLS is the wrong protocol to use
        // Thus, we forward the data to a TCP tunnel
        owner.switch_service<tunnel::tunnel_service>();
    }

    void tls_service::handle_client_hello() {
        try {
            client_hello_msg = std::make_unique<handshake::client_hello>(
                std::move(handshake::client_hello::from_raw_data(client_hello_reader.get_bytes())));

            connect_server();
        }
        catch (const error::tls::invalid_client_hello_exception &) {
            handle_not_client_hello();
        }
    }

    void tls_service::connect_server() {
        connect_server_async(boost::bind(&tls_service::on_connect_server, this, 
            boost::asio::placeholders::error));
    }

    void tls_service::on_connect_server(const boost::system::error_code &error) {
        if (error != boost::system::errc::success) {
            flow.error.set_boost_error(error);
            flow.error.set_proxy_error(errc::upstream_connect_error);
            flow.error.set_message(out::string::stream("Could not connect to ", flow.server.get_host(), ':', flow.server.get_port()));
            interceptors.tls.run(intercept::tls_event::error, flow);
            // Establish TLS with client and report the error later
            establish_tls_with_client();
        }
        else {
            establish_tls_with_server();
        }
    }

    void tls_service::establish_tls_with_server() {
        if (!client_hello_msg) {
            throw error::tls::tls_service_error_exception { "Must parse Client Hello message before establishing TLS with server" };
        }

        auto method = program::options::instance().ssl_server_method;
        ssl_client_context_args.reset(
            new openssl::ssl_context_args {
                program::options::instance().ssl_verify,
                method,
                openssl::ssl_context_args::get_options_for_method(method),
                client_store->cert_file()
            }
        );

        if (client_hello_msg->has_alpn_extension() && !program::options::instance().ssl_negotiate_alpn) {
            // Remove unsupported protocols to be sure the server picks one we can read
            std::copy_if(client_hello_msg->alpn.begin(), client_hello_msg->alpn.end(), std::back_inserter(ssl_client_context_args->alpn_protos),
                [](const std::string &protocol) {
                    return !((protocol.rfind("h2-") == 0) || (protocol == "SPDY"));
                });
            // TODO: Remove this line once HTTP 2 is supported
            ssl_client_context_args->alpn_protos.erase(std::remove(ssl_client_context_args->alpn_protos.begin(), ssl_client_context_args->alpn_protos.end(), "h2"), ssl_client_context_args->alpn_protos.end());
        }

        // If client TLS is established already, use client's negotiated ALPN by default
        if (flow.client.secured()) {
            ssl_client_context_args->alpn_protos.clear();
            ssl_client_context_args->alpn_protos.push_back(flow.client.get_alpn());
        }

        if (!program::options::instance().ssl_negotiate_ciphers) {
            // Use only ciphers we have named with the server
            std::copy_if(client_hello_msg->cipher_suites.begin(), client_hello_msg->cipher_suites.end(), std::back_inserter(ssl_client_context_args->cipher_suites),
                [](const handshake::cipher_suite_name &cipher) {
                    return handshake::is_valid(cipher);
                });
        }

        flow.establish_tls_with_server_async(*ssl_client_context_args, boost::bind(&tls_service::on_establish_tls_with_server, this,
            boost::asio::placeholders::error));
    }

    void tls_service::on_establish_tls_with_server(const boost::system::error_code &error) {
        if (error != boost::system::errc::success) {
            flow.error.set_boost_error(error);
            flow.error.set_proxy_error(errc::upstream_handshake_failed);
            flow.error.set_message(out::string::stream("Could not establish TLS with ", flow.server.get_host(), ':', flow.server.get_port()));
            interceptors.tls.run(intercept::tls_event::error, flow);
            // Establish TLS with client and report the error later
            establish_tls_with_client();
        }
        else {
            establish_tls_with_client();
        }
    }

    bool accept_all(bool preverified, boost::asio::ssl::verify_context &ctx) {
        return true;
    }

    int alpn_select_callback(SSL *ssl, const unsigned char **out, unsigned char *outlen, const unsigned char *in, unsigned int inlen, void *arg) {
        // in contains protos to select from
        // We must set out and outlen to the select ALPN
        
        std::string &&protos = reinterpret_cast<const char *>(in);
        std::size_t pos = 0;

        // Use ALPN already negotiated
        if (arg != nullptr) {
            std::string &&server_alpn = reinterpret_cast<const char *>(arg);
            if ((pos = protos.find(server_alpn)) != std::string::npos) {
                *out = in + pos;
                *outlen = static_cast<unsigned int>(server_alpn.length());
                return SSL_TLSEXT_ERR_OK;
            }
        }

        // Use default ALPN
        if ((pos = protos.find(tls_service::default_alpn)) != std::string::npos) {
            *out = in + pos;
            *outlen = static_cast<unsigned int>(tls_service::default_alpn.length());
        }
        // Use first option
        else {
            *outlen = static_cast<unsigned int>(*in);
            *out = in + 1;
        }

        return SSL_TLSEXT_ERR_OK;
    }

    void tls_service::establish_tls_with_client() {
        auto cert = get_certificate_for_client();
        auto method = program::options::instance().ssl_client_method;
        ssl_server_context_args.reset(
            new openssl::ssl_server_context_args {
                openssl::ssl_context_args {
                    boost::asio::ssl::verify_none,
                    method,
                    openssl::ssl_context_args::get_options_for_method(method),
                    cert.chain_file,
                    accept_all,
                    default_client_ciphers,
                    { },
                    alpn_select_callback,
                    flow.server.secured() ? flow.server.get_alpn() : std::optional<std::string> { }
                },
                cert.cert,
                cert.pkey,
                server_store->get_dhparams()
            }
        );

        if (flow.server.connected() && flow.server.secured()) {
            ssl_server_context_args->cert_chain = flow.server.get_cert_chain();
        }

        flow.establish_tls_with_client_async(*ssl_server_context_args, boost::bind(&tls_service::on_establish_tls_with_client, this,
            boost::asio::placeholders::error));
    }

    tcp::tls::x509::memory_certificate tls_service::get_certificate_for_client() {
        // Information that may be needed for the client certificate
        std::optional<std::string> host;
        std::set<std::string> sans;
        std::optional<std::string> organization;

        if (flow.server.connected()) {
            // TLS is established, using certificate data
            if (flow.server.secured()) {
                auto cert = flow.server.get_cert();
                auto cert_sans = cert.sans();
                std::copy(cert_sans.begin(), cert_sans.end(), std::inserter(sans, sans.end()));

                auto cn = cert.common_name();
                if (cn.has_value()) {
                    sans.insert(cn.value());
                    host = cn.value();
                }

                auto org = cert.organization();
                if (org.has_value()) {
                    organization = org.value();
                }
            }
            else {
                host = flow.server.get_host();
            }
        }

        // Copy Client Hello SNI entries 
        for (const auto &san : client_hello_msg->server_names) {
            sans.insert(san.host_name);
        }

        if (host.has_value()) {
            sans.insert(host.value());
        }

        return server_store->get_certificate(host, sans, organization);
    }

    void tls_service::on_establish_tls_with_client(const boost::system::error_code &error) {
        if (error != boost::system::errc::success) {
            // TODO: What is causing these handshake errors?
            out::safe_console::log("Client handshake error:", error.message());

            // Client is not going to continue if its handshake failed
            flow.error.set_boost_error(error);
            flow.error.set_proxy_error(errc::downstream_handshake_failed);
            interceptors.tls.run(intercept::tls_event::error, flow);
            stop();
        }
        else {
            // TLS is successfully established within the connection objects
            interceptors.tls.run(intercept::tls_event::established, flow);
            std::string alpn = flow.client.get_alpn();
            if (alpn == "http/1.1") {
                // Any TLS errors are reported to the client by the HTTP service
                owner.switch_service<http::http1::http_service>();
            }
            else if (!flow.error.has_error()) {
                owner.switch_service<tunnel::tunnel_service>();
            } 
            else {
                stop();
            }
        }
    }

    void tls_service::create_cert_store() {
        client_store.reset();
        server_store.reset();
        client_store = std::make_unique<x509::client_store>();
        server_store = std::make_unique<x509::server_store>();
    }
}