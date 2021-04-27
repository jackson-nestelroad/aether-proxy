/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/filesystem.hpp>
#include <boost/asio/ssl.hpp>

#include <aether/program/options.hpp>
#include <aether/proxy/error/exceptions.hpp>
#include <aether/util/singleton.hpp>
#include <aether/proxy/tcp/tls/openssl/smart_ptrs.hpp>

namespace proxy::tcp::tls::x509 {
    /*
        Class for a X.509 certificate store to be used by SSL clients.
    */
    class client_store {
    private:
        std::string trusted_certificates_file;

    public:
        static const boost::filesystem::path default_trusted_certificates_file;

        client_store();
        const std::string &cert_file() const;
    };
}