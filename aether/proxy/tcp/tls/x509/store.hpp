/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/noncopyable.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio/ssl.hpp>

#include <aether/proxy/error/exceptions.hpp>

namespace proxy::tcp::tls::x509 {
    /*
        Wrapper class for a X.509 certificate store.
    */
    class store 
        : boost::noncopyable {
    private:
        static std::unique_ptr<store> singleton;

        static boost::filesystem::path trusted_certificates_file;

        X509_STORE *native;

    public:
        static const boost::filesystem::path default_trusted_certificates_file;

        store();
        X509_STORE *native_handle() const;

        static store &get_singleton();
        static void set_trusted_certificates_file(const std::string &path);
    };
}