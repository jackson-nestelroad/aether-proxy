/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio/ssl.hpp>

namespace proxy::tcp::tls::x509 {
    /*
        Wrapper class for a X.509 certificate.
    */
    class certificate {
    private:
        X509 *native;

    public:
        certificate(SSL *ssl);
        certificate(X509 *cert);
        ~certificate();

        certificate(const certificate &other);
        certificate &operator=(const certificate &other);
    };
}