/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "client_store.hpp"
#include <aether/proxy/server_components.hpp>

namespace proxy::tcp::tls::x509 {
    const boost::filesystem::path client_store::default_trusted_certificates_file = (boost::filesystem::path(AETHER_HOME) / "cert_store/mozilla-cacert.pem").make_preferred();

    client_store::client_store(server_components &components)
        : options(components.options) 
    {
        trusted_certificates_file = options.ssl_verify_upstream_trusted_ca_file_path;
    }

    const std::string &client_store::cert_file() const {
        return trusted_certificates_file;
    }
}