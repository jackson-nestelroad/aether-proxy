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
}