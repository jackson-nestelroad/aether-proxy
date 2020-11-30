/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <optional>
#include <boost/asio/ssl.hpp>

#include <aether/proxy/error/exceptions.hpp>
#include <aether/proxy/tcp/tls/openssl/smart_ptrs.hpp>

namespace proxy::tcp::tls::x509 {
    /*
        Wrapper class for a X.509 certificate.
    */
    class certificate 
        : public openssl::ptrs::x509 {
    private:
        std::optional<std::string> get_nid_from_name(int nid);

    public:
        using serial_t = long;

        using openssl::ptrs::x509::x509;
        certificate(SSL *ssl);

        /*
            Retrieves the certificate's common name.
        */
        std::optional<std::string> common_name();

        /*
            Retrieves the certificate's organization.
        */
        std::optional<std::string> organization();

        /*
            Retrieves a list of the certificate's subject alternative names (SANs).
        */
        std::vector<std::string> sans();
    };
}