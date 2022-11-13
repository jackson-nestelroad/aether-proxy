/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <string>

#include "aether/proxy/tcp/tls/openssl/smart_ptrs.hpp"
#include "aether/proxy/tcp/tls/x509/certificate.hpp"

namespace proxy::tcp::tls::x509 {

// Data structure for a single in-memory X.509 certificate.
struct memory_certificate {
  certificate cert;
  openssl::ptrs::evp_pkey pkey;
  std::string chain_file;
  // std::set<std::string> sans;
};

}  // namespace proxy::tcp::tls::x509
