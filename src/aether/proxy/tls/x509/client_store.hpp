/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio/ssl.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <string_view>

#include "aether/program/options.hpp"
#include "aether/proxy/error/exceptions.hpp"
#include "aether/proxy/tls/openssl/smart_ptrs.hpp"

namespace proxy {
class server_components;
}

namespace proxy::tls::x509 {
/*
    Class for a X.509 certificate store to be used by SSL clients.
*/
class client_store {
 public:
  static const boost::filesystem::path default_trusted_certificates_file;

  client_store(server_components& components);
  client_store() = delete;
  ~client_store() = default;
  client_store(const client_store& other) = delete;
  client_store& operator=(const client_store& other) = delete;
  client_store(client_store&& other) noexcept = delete;
  client_store& operator=(client_store&& other) noexcept = delete;

  inline std::string_view cert_file() const { return trusted_certificates_file_; }

 private:
  program::options& options_;
  std::string trusted_certificates_file_;
};

}  // namespace proxy::tls::x509
