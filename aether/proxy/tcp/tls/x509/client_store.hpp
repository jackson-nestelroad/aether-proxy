/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/filesystem.hpp>
#include <boost/asio/ssl.hpp>

#include <aether/proxy/error/exceptions.hpp>
#include <aether/proxy/tcp/tls/openssl/smart_ptrs.hpp>
#include <aether/program/options.hpp>

namespace proxy {
    class server_components;
}

namespace proxy::tcp::tls::x509 {
    /*
        Class for a X.509 certificate store to be used by SSL clients.
    */
    class client_store {
    private:
        program::options &options;
        std::string trusted_certificates_file;

    public:
        static const boost::filesystem::path default_trusted_certificates_file;

        client_store(server_components &components);
        client_store() = delete;
        ~client_store() = default;
        client_store(const client_store &other) = delete;
        client_store &operator=(const client_store &other) = delete;
        client_store(client_store &&other) noexcept = delete;
        client_store &operator=(client_store &&other) noexcept = delete;

        const std::string &cert_file() const;
    };
}