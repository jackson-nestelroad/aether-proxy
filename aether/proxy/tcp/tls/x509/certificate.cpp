/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "certificate.hpp"

namespace proxy::tcp::tls::x509 {
    certificate::certificate(SSL *ssl) {
        native = SSL_get_peer_certificate(ssl);
        X509_up_ref(native);
    }

    certificate::certificate(X509 *cert) {
        native = cert;
        X509_up_ref(native);
    }

    certificate::~certificate() {
        if (native != nullptr) {
            X509_free(native);
        }
    }

    certificate::certificate(const certificate &other) 
        : native(other.native) {
        X509_up_ref(native);
    }

    certificate &certificate::operator=(const certificate &other) {
        native = other.native;
        X509_up_ref(native);
        return *this;
    }

    X509 *certificate::native_handle() {
        return native;
    }
}