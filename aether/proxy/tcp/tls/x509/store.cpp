/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "store.hpp"

namespace proxy::tcp::tls::x509 {
    const boost::filesystem::path store::default_trusted_certificates_file = (boost::filesystem::path(AETHER_HOME) / "cert_store/cacert.pem").make_preferred();
    boost::filesystem::path store::trusted_certificates_file = store::default_trusted_certificates_file;

    std::unique_ptr<store> store::singleton { };

    store::store() {
        native = X509_STORE_new();
        int res = X509_STORE_load_locations(native, trusted_certificates_file.string().c_str(), NULL);
        if (!res) {
            throw error::tls::invalid_trusted_certificates_file { };
        }
    }

    X509_STORE *store::native_handle() const {
        X509_STORE_up_ref(native);
        return native;
    }

    store &store::get_singleton() {
        if (!singleton) {
            singleton = std::make_unique<store>();
        }
        return *singleton;
    }

    void store::set_trusted_certificates_file(const std::string &path) {
        trusted_certificates_file = path;
    }
}