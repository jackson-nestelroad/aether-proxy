/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio/ssl.hpp>

#include <aether/proxy/tcp/tls/openssl/smart_ptrs.hpp>

namespace proxy::tcp::tls::x509 {
    /*
        Wrapper class for a X.509 certificate.
    */
    class certificate 
        : public openssl::ptrs::x509 {
    public:
        using openssl::ptrs::x509::x509;
        certificate(SSL *ssl);
    };
}