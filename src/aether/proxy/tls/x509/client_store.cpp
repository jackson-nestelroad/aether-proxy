/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "client_store.hpp"

#include <filesystem>

#include "aether/proxy/server_components.hpp"

namespace proxy::tls::x509 {
const std::filesystem::path client_store::default_trusted_certificates_file =
    (std::filesystem::path(AETHER_HOME) / "cert_store/mozilla-cacert.pem").make_preferred();

client_store::client_store(server_components& components) : options_(components.options) {
  trusted_certificates_file_ = options_.ssl_verify_upstream_trusted_ca_file_path;
}
}  // namespace proxy::tls::x509
