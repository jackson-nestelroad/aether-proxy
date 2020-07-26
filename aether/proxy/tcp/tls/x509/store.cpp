/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "store.hpp"

namespace proxy::tcp::tls::x509 {
    const boost::filesystem::path store::default_trusted_certificates_file = (boost::filesystem::path(AETHER_HOME) / "cert_store/cacert.pem").make_preferred();

    void store::init() {
        trusted_certificates_file = program::options::instance().ssl_verify_upstream_trusted_ca_file_path;
        native = X509_STORE_new();
        if (!X509_STORE_load_locations(native, trusted_certificates_file.c_str(), NULL)) {
            throw error::tls::invalid_trusted_certificates_file { };
        }
    }

    void store::cleanup() {
        if (native) {
            X509_STORE_free(native);
        }
    }

    X509_STORE *store::native_handle() const {
        X509_STORE_up_ref(native);
        return native;
    }
}