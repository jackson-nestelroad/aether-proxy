/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <openssl/ssl.h>

#include <optional>
#include <set>
#include <string>
#include <vector>

#include "aether/proxy/error/error.hpp"
#include "aether/proxy/tls/openssl/openssl_ptrs.hpp"

namespace proxy::tls::x509 {

// Wrapper class for a X.509 certificate.
class certificate : public openssl::ptrs::x509 {
 public:
  using serial_t = long;
  using openssl::ptrs::x509::x509;

  certificate(SSL* ssl);

  // Retrieves the certificate's common name.
  result<std::optional<std::string>> common_name();

  // Retrieves the certificate's organization.
  result<std::optional<std::string>> organization();

  // Retrieves a list of the certificate's subject alternative names (SANs).
  std::vector<std::string> sans();

  // Generates a list of all server names this certificate is valid for.
  std::vector<std::string> all_server_names();

 private:
  result<std::optional<std::string>> get_nid_from_name(int nid);

  void add_sans(std::vector<std::string>& names);
};

// Interface for finding and creating a X.509 certificate.
struct certificate_interface {
  std::optional<std::string> common_name;
  std::set<std::string> sans;
  std::optional<std::string> organization;
};

}  // namespace proxy::tls::x509
