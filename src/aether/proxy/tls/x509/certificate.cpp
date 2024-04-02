/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "certificate.hpp"

#include <openssl/ssl.h>

#include <optional>
#include <set>
#include <string>
#include <vector>

#include "aether/proxy/tls/openssl/openssl_ptrs.hpp"

namespace proxy::tls::x509 {

certificate::certificate(SSL* ssl) : openssl::ptrs::x509(openssl::ptrs::wrap_unique, SSL_get_peer_certificate(ssl)) {}

result<std::optional<std::string>> certificate::get_nid_from_name(int nid) {
  X509_NAME* name = X509_get_subject_name(native_);
  if (!name) {
    return error::tls::certificate_issuer_not_found();
  }

  int index = X509_NAME_get_index_by_NID(name, nid, -1);

  // This entry does not exist.
  if (index < 0) {
    return std::nullopt;
  }

  X509_NAME_ENTRY* common_name_entry = X509_NAME_get_entry(name, index);
  if (!common_name_entry) {
    return error::tls::certificate_name_entry_error();
  }

  ASN1_STRING* common_name = X509_NAME_ENTRY_get_data(common_name_entry);
  if (!common_name) {
    return error::tls::certificate_name_entry_error();
  }

  unsigned char* str = nullptr;
  int asn1_length = ASN1_STRING_to_UTF8(&str, common_name);
  if (!str || asn1_length == 0) {
    return error::tls::certificate_name_entry_error();
  }

  auto result = std::string(str, str + asn1_length);
  OPENSSL_free(str);
  return result;
}

result<std::optional<std::string>> certificate::common_name() { return get_nid_from_name(NID_commonName); }

result<std::optional<std::string>> certificate::organization() { return get_nid_from_name(NID_organizationName); }

std::vector<std::string> certificate::sans() {
  std::vector<std::string> names;
  openssl::ptrs::general_names names_ext(
      openssl::ptrs::wrap_unique,
      static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(native_, NID_subject_alt_name, nullptr, nullptr)));
  if (!names_ext) {
    return names;
  }

  int num_names = sk_GENERAL_NAME_num(*names_ext);
  for (int i = 0; i < num_names; ++i) {
    auto entry = sk_GENERAL_NAME_value(*names_ext, i);
    // Ignore null entries or non-DNS entries.
    if (!entry || entry->type != GEN_DNS) {
      continue;
    }

    unsigned char* str = nullptr;
    int asn1_length = ASN1_STRING_to_UTF8(&str, entry->d.dNSName);

    if (str && asn1_length != 0) {
      names.push_back(std::string(str, str + asn1_length));
    }

    OPENSSL_free(str);
  }

  return names;
}

}  // namespace proxy::tls::x509
