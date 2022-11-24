/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio/ssl.hpp>
#include <filesystem>
#include <map>
#include <mutex>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <string_view>

#include "aether/program/options.hpp"
#include "aether/program/properties.hpp"
#include "aether/proxy/constants/server_constants.hpp"
#include "aether/proxy/tls/openssl/smart_ptrs.hpp"
#include "aether/proxy/tls/x509/certificate.hpp"
#include "aether/proxy/tls/x509/memory_certificate.hpp"
#include "aether/util/string_view.hpp"

namespace proxy {
class server_components;
}

namespace proxy::tls::x509 {

// Class for a X.509 certificate store to be used by the SSL server.
// Responsible for SSL certificate generation and storage.
class server_store {
 public:
  static const std::filesystem::path default_dir;
  static const std::filesystem::path default_properties_file;
  static const std::filesystem::path default_dhparam_file;

  server_store(server_components& components);
  server_store() = delete;
  ~server_store() = default;
  server_store(const server_store& other) = delete;
  server_store& operator=(const server_store& other) = delete;
  server_store(server_store&& other) noexcept = delete;
  server_store& operator=(server_store&& other) noexcept = delete;

  inline openssl::ptrs::evp_pkey& dhpkey() { return dhpkey_; }

  std::optional<memory_certificate> get_certificate(const certificate_interface& cert_interface);
  memory_certificate create_certificate(const certificate_interface& cert_interface);

 private:
  static constexpr int default_key_size = 2048;
  static constexpr long default_expiry_time = 60 * 60 * 24 * 365 * 3;
  static constexpr std::size_t max_num_certs = 100;

  static constexpr std::string_view ca_pkey_file_suffix = "-cakey.pem";
  static constexpr std::string_view ca_cert_file_suffix = "-cacert.pem";

  static constexpr std::string_view ca_pkey_file_name =
      util::string_view::join_v<proxy::constants::lowercase_name, ca_pkey_file_suffix>;
  static constexpr std::string_view ca_cert_file_name =
      util::string_view::join_v<proxy::constants::lowercase_name, ca_cert_file_suffix>;

  void read_store(const std::string& pkey_path, const std::string& cert_path);

  void create_store(const std::string& dir);
  void create_ca();
  void add_cert_name_entry_from_props(X509_NAME* name, int entry_code, std::string_view prop_name);
  void add_cert_name_entry_from_props(X509_NAME* name, int entry_code, std::string_view prop_name,
                                      std::string_view default_value);
  void add_cert_extension(certificate& cert, int ext_id, std::string_view value);

  void load_dhpkey();

  memory_certificate& insert(const std::string& key, memory_certificate cert);
  std::vector<std::string> get_asterisk_forms(const std::string& domain);
  certificate::serial_t generate_serial();
  certificate generate_certificate(const certificate_interface& cert_interface);

  program::options& options_;
  program::properties props_;

  std::string ca_pkey_file_fullpath_;
  std::string ca_cert_file_fullpath_;

  openssl::ptrs::evp_pkey pkey_;
  certificate default_cert_;
  openssl::ptrs::evp_pkey dhpkey_;

  // Guards access to certificate data structures.
  std::mutex cert_data_mutex_;

  // Maps a common name or domain name to an in-memory certificate.
  std::map<std::string, memory_certificate> cert_map_;

  // The top of the queue is the next certificate to delete when the max capacity is reached.
  std::queue<std::string> cert_queue_;
};

}  // namespace proxy::tls::x509
