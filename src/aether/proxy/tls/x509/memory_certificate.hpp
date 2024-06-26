/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <string>
#include <vector>

#include "aether/proxy/tls/openssl/openssl_ptrs.hpp"
#include "aether/proxy/tls/x509/certificate.hpp"

namespace proxy::tls::x509 {

// Data structure for a single in-memory X.509 certificate.
struct memory_certificate {
  certificate cert;
  openssl::ptrs::evp_pkey pkey;
  std::string chain_file;
  std::vector<std::string> names;
};

}  // namespace proxy::tls::x509
