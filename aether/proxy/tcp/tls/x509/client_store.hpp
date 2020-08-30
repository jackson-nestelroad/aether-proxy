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

namespace proxy::tcp::tls::x509 {
    /*
        Wrapper class for a X.509 certificate store to be used by SSL clients.
    */
    class client_store 
        : public util::const_singleton<client_store> {
    protected:
        std::string trusted_certificates_file;
        X509_STORE *native;

    public:
        static const boost::filesystem::path default_trusted_certificates_file;

        void init();
        void cleanup();

        X509_STORE *native_handle() const;
    };
}