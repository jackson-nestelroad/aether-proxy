/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "client_store.hpp"

namespace proxy::tcp::tls::x509 {
    const boost::filesystem::path client_store::default_trusted_certificates_file = (boost::filesystem::path(AETHER_HOME) / "cert_store/mozilla-cacert.pem").make_preferred();

    client_store::client_store() {
        trusted_certificates_file = program::options::instance().ssl_verify_upstream_trusted_ca_file_path;
    }

    const std::string &client_store::cert_file() const {
        return trusted_certificates_file;
    }
}