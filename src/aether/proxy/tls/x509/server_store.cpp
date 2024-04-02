/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "server_store.hpp"

#include <algorithm>
#include <filesystem>
#include <limits>
#include <mutex>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "aether/program/options.hpp"
#include "aether/program/properties.hpp"
#include "aether/proxy/constants/server_constants.hpp"
#include "aether/proxy/error/exceptions.hpp"
#include "aether/proxy/server_components.hpp"
#include "aether/proxy/tls/openssl/openssl_ptrs.hpp"
#include "aether/proxy/tls/x509/certificate.hpp"
#include "aether/proxy/tls/x509/memory_certificate.hpp"
#include "aether/util/result_macros.hpp"
#include "aether/util/string_view.hpp"

namespace proxy::tls::x509 {
const std::filesystem::path server_store::default_dir =
    (std::filesystem::path(AETHER_HOME) / "cert_store").make_preferred();

const std::filesystem::path server_store::default_properties_file = (default_dir / "proxy.properties").make_preferred();

const std::filesystem::path server_store::default_dhparam_file = (default_dir / "dhparam.default.pem").make_preferred();

result<std::unique_ptr<server_store>> server_store::create(server_components& components) {
  std::unique_ptr<server_store> store(new server_store(components));
  // Get properties.
  RETURN_IF_ERROR(
      store->props_.parse_file(store->options_.ssl_cert_store_properties).map_err([](util::generic_error error) {
        return error::error_state(error);
      }));

  const auto& dir = store->options_.ssl_cert_store_dir;
  auto ca_pkey_path = std::filesystem::path(dir) / ca_pkey_file_name.data();
  auto ca_cert_path = std::filesystem::path(dir) / ca_cert_file_name.data();

  store->ca_pkey_file_fullpath_ = ca_pkey_path.string();
  store->ca_cert_file_fullpath_ = ca_cert_path.string();

  // Both files exist, use them.
  if (std::filesystem::exists(ca_pkey_path) && std::filesystem::exists(ca_cert_path)) {
    RETURN_IF_ERROR(store->read_store(ca_pkey_path.string(), ca_cert_path.string()));
  } else {
    // Need to generate and save the SSL data.
    RETURN_IF_ERROR(store->create_store(dir));
  }
  RETURN_IF_ERROR(store->load_dhpkey());
  return store;
}

server_store::server_store(server_components& components)
    : options_(components.options), pkey_(nullptr), default_cert_(openssl::ptrs::in_place), dhpkey_(nullptr) {}

result<void> server_store::read_store(const std::string& pkey_path, const std::string& cert_path) {
  // Get password from properties.
  auto password_prop = props_.get("password");
  char* password = nullptr;
  if (password_prop.has_value()) {
    password = const_cast<char*>(password_prop.value().data());
  }

  // Read private key.
  openssl::ptrs::unique_native_file ca_pkey_file = nullptr;
  if (ca_pkey_file.open(pkey_path.data(), "rb") || *ca_pkey_file == nullptr) {
    return error::tls::ssl_server_store_creation_error(
        out::string::stream("Could not open ", pkey_path, " for reading."));
  }
  pkey_ = openssl::ptrs::evp_pkey(openssl::ptrs::wrap_unique,
                                  PEM_read_PrivateKey(*ca_pkey_file, nullptr, nullptr, password));
  if (!pkey_) {
    return error::tls::ssl_server_store_creation_error("Failed to read existing private key.");
  }

  // Read server certificate.
  openssl::ptrs::unique_native_file ca_cert_file = nullptr;
  if (ca_cert_file.open(cert_path.data(), "rb") || *ca_cert_file == nullptr) {
    return error::tls::ssl_server_store_creation_error(
        out::string::stream("Could not open ", cert_path, " for reading."));
  }
  default_cert_ = certificate(openssl::ptrs::wrap_unique, PEM_read_X509(*ca_cert_file, nullptr, nullptr, password));
  if (!default_cert_) {
    return error::tls::ssl_server_store_creation_error("Failed to read existing certificate file.");
  }
  return util::ok;
}

result<void> server_store::create_store(const std::string& dir) {
  if (!std::filesystem::exists(dir)) {
    std::filesystem::create_directory(dir);
  }

  // Generate private key and certificate.
  RETURN_IF_ERROR(create_ca());

  // Get password from properties.
  auto password_prop = props_.get("password");
  unsigned char* password = nullptr;
  int password_len = 0;
  if (password_prop.has_value()) {
    password = const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(password_prop.value().data()));
    password_len = static_cast<int>(password_prop.value().length());
  }

  int error = 0;

  // Write private key to disk.
  openssl::ptrs::unique_native_file ca_pkey_file = nullptr;
  if (ca_pkey_file.open(ca_pkey_file_fullpath_.data(), "wb") || *ca_pkey_file == nullptr) {
    return error::tls::ssl_server_store_creation_error(
        out::string::stream("Could not open ", ca_pkey_file_fullpath_, " for writing."));
  }
  error = PEM_write_PrivateKey(*ca_pkey_file, *pkey_, EVP_des_ede3_cbc(), password, password_len, nullptr, nullptr);
  if (!error) {
    return error::tls::ssl_server_store_creation_error("Failed to write private key to disk.");
  }

  // Write certificate to disk
  openssl::ptrs::unique_native_file ca_cert_file = nullptr;
  if (ca_cert_file.open(ca_cert_file_fullpath_.data(), "wb") || *ca_cert_file == nullptr) {
    return error::tls::ssl_server_store_creation_error(
        out::string::stream("Could not open ", ca_cert_file_fullpath_, " for writing."));
  }
  error = PEM_write_X509(*ca_cert_file, *default_cert_);
  if (!error) {
    return error::tls::ssl_server_store_creation_error("Failed to write certificate to disk.");
  }
  return util::ok;
}

result<void> server_store::create_ca() {
  auto key_size_str = props_.get("key_size");
  int key_size = default_key_size;
  if (key_size_str.has_value()) {
    try {
      key_size = boost::lexical_cast<int>(key_size_str.value());
    } catch (boost::bad_lexical_cast&) {
      // Do nothing.
    }
  }

  // Generate an RSA private key.
  pkey_ = openssl::ptrs::evp_pkey(openssl::ptrs::wrap_unique, EVP_RSA_gen(key_size));

  if (!X509_set_version(*default_cert_, 2)) {
    return error::tls::ssl_server_store_creation_error("Error setting certificate version.");
  }

  if (!ASN1_INTEGER_set(X509_get_serialNumber(*default_cert_), generate_serial())) {
    return error::tls::ssl_server_store_creation_error("Error setting certificate serial number.");
  }

  if (!X509_gmtime_adj(X509_get_notBefore(*default_cert_), 0)) {
    return error::tls::ssl_server_store_creation_error("Error setting certificate's notBefore property.");
  }

  if (!X509_gmtime_adj(X509_get_notAfter(*default_cert_), default_expiry_time)) {
    return error::tls::ssl_server_store_creation_error("Error setting certificate's notAfter property.");
  }

  // Attach properties to certificate.
  X509_NAME* name = X509_get_subject_name(*default_cert_);
  RETURN_IF_ERROR(
      add_cert_name_entry_from_props(name, NID_commonName, "name", proxy::constants::lowercase_name.data()));
  RETURN_IF_ERROR(add_cert_name_entry_from_props(name, NID_countryName, "country"));
  RETURN_IF_ERROR(add_cert_name_entry_from_props(name, NID_stateOrProvinceName, "state"));
  RETURN_IF_ERROR(add_cert_name_entry_from_props(name, NID_localityName, "locality"));
  RETURN_IF_ERROR(add_cert_name_entry_from_props(name, NID_organizationName, "organization"));
  RETURN_IF_ERROR(add_cert_name_entry_from_props(name, NID_organizationalUnitName, "organizational_unit"));
  RETURN_IF_ERROR(add_cert_name_entry_from_props(name, NID_dnQualifier, "dn_qualifier"));

  // Self-signed.
  if (!X509_set_issuer_name(*default_cert_, name)) {
    return error::tls::ssl_server_store_creation_error("Error setting certificate's issuer.");
  }

  if (!X509_set_pubkey(*default_cert_, *pkey_)) {
    return error::tls::ssl_server_store_creation_error("Error setting certificate public key.");
  }

  RETURN_IF_ERROR(add_cert_extension(default_cert_, NID_basic_constraints, "critical,CA:TRUE"));
  RETURN_IF_ERROR(add_cert_extension(default_cert_, NID_netscape_cert_type, "sslCA"));
  RETURN_IF_ERROR(add_cert_extension(default_cert_, NID_ext_key_usage,
                                     "serverAuth,clientAuth,emailProtection,timeStamping,"
                                     "msCodeInd,msCodeCom,msCTLSign,msSGC,msEFS,nsSGC"));
  RETURN_IF_ERROR(add_cert_extension(default_cert_, NID_key_usage, "critical,keyCertSign,cRLSign"));
  RETURN_IF_ERROR(add_cert_extension(default_cert_, NID_subject_key_identifier, "hash"));

  if (!X509_sign(*default_cert_, *pkey_, EVP_sha256())) {
    return error::tls::ssl_server_store_creation_error("Error signing certificate.");
  }
  return util::ok;
}

result<void> server_store::add_cert_name_entry_from_props(X509_NAME* name, int entry_code, std::string_view prop_name) {
  auto prop = props_.get(prop_name);
  if (prop.has_value()) {
    if (!X509_NAME_add_entry_by_NID(name, entry_code, MBSTRING_ASC,
                                    reinterpret_cast<const unsigned char*>(prop.value().data()), -1, -1, 0)) {
      return error::tls::ssl_server_store_creation_error(
          out::string::stream("Error setting certificate's ", prop_name, " property."));
    }
  }
  return util::ok;
}

result<void> server_store::add_cert_name_entry_from_props(X509_NAME* name, int entry_code, std::string_view prop_name,
                                                          std::string_view default_value) {
  if (!X509_NAME_add_entry_by_NID(
          name, entry_code, MBSTRING_ASC,
          reinterpret_cast<const unsigned char*>(props_.get(prop_name).value_or(default_value.data()).data()), -1, -1,
          0)) {
    return error::tls::ssl_server_store_creation_error(
        out::string::stream("Error setting certificate's ", prop_name, " property."));
  }
  return util::ok;
}

result<void> server_store::add_cert_extension(certificate& cert, int ext_id, std::string_view value) {
  X509V3_CTX ctx{};
  X509V3_set_ctx_nodb(&ctx);
  X509V3_set_ctx(&ctx, *cert, *cert, nullptr, nullptr, 0);

  openssl::ptrs::x509_extension ext(openssl::ptrs::wrap_unique,
                                    X509V3_EXT_conf_nid(nullptr, &ctx, ext_id, value.data()));

  if (!X509_add_ext(*cert, *ext, -1)) {
    return error::tls::ssl_server_store_creation_error("Error adding certificate extension.");
  }
  return util::ok;
}

result<void> server_store::load_dhpkey() {
  const auto& dhparam_file_name = options_.ssl_dhparam_file;

  // We need this file to load the dhpkey.
  // It is much too slow to generate this in the program itself, so the program should be run with the parameters
  // already generated. This file can be generated using `openssl dhparam`.
  if (!std::filesystem::exists(dhparam_file_name)) {
    // Default file was not found.
    if (dhparam_file_name == default_dhparam_file) {
      return error::tls::ssl_server_store_creation_error(
          "The server's Diffie-Hellman parameters file was not found. This is a fatal error, and it is likely a result "
          "of the program not being set up correctly.");
    } else {
      return error::tls::ssl_server_store_creation_error("Could not find Diffie-Hellman parameters file at " +
                                                         dhparam_file_name);
    }
  }

  openssl::ptrs::unique_native_file dhparam_file = nullptr;
  if (dhparam_file.open(dhparam_file_name.data(), "rb") || *dhparam_file == nullptr) {
    return error::tls::ssl_server_store_creation_error(
        out::string::stream("Failed to open ", dhparam_file_name, " for reading."));
  }

  openssl::ptrs::bio dhparam_bio(openssl::ptrs::wrap_unique, std::move(dhparam_file));

  EVP_PKEY* dhpkey = nullptr;
  openssl::ptrs::dh_decoder_context decoder_context(openssl::ptrs::in_place, &dhpkey);

  if (decoder_context == nullptr) {
    return error::tls::ssl_server_store_creation_error("Failed to create Diffie-Hellman private key decoder.");
  }

  if (!OSSL_DECODER_from_bio(*decoder_context, *dhparam_bio)) {
    return error::tls::ssl_server_store_creation_error(
        "Failed to read and decode Diffie-Hellman parameters from disk.");
  }

  dhpkey_ = openssl::ptrs::evp_pkey(openssl::ptrs::wrap_unique, dhpkey);
  return util::ok;
}

memory_certificate& server_store::insert(const std::string& key, memory_certificate cert) {
  std::lock_guard<std::mutex> lock(cert_data_mutex_);
  auto& result = cert_map_.emplace(key, std::move(cert)).first->second;
  cert_queue_.push(key);
  if (cert_map_.size() > max_num_certs) {
    cert_map_.erase(cert_queue_.front());
    cert_queue_.pop();
  }
  return result;
}

std::vector<std::string> server_store::get_asterisk_forms(const std::string& domain) {
  std::vector<std::string> asterisk_forms{domain};
  std::size_t prev = 0;
  std::size_t pos = 0;
  while ((pos = domain.find('.', prev)) != std::string::npos) {
    prev = pos + 1;
    asterisk_forms.push_back("*." + domain.substr(prev));
  }
  return asterisk_forms;
}

template <typename Set1, typename Set2>
bool is_disjoint(const Set1& set1, const Set2& set2) {
  if (set1.empty() || set2.empty()) {
    return true;
  }

  typename Set1::const_iterator it1 = set1.begin(), it1_end = set1.end();
  typename Set2::const_iterator it2 = set2.begin(), it2_end = set2.end();

  if (*it1 > *set2.rbegin() || *it2 > *set1.rbegin()) {
    return true;
  }

  while (it1 != it1_end && it2 != it2_end) {
    if (*it1 == *it2) {
      return false;
    }
    if (*it1 < *it2) {
      ++it1;
    } else {
      ++it2;
    }
  }

  return true;
}

std::optional<memory_certificate> server_store::get_certificate(const certificate_interface& cert_interface) {
  std::set<std::string> keys;
  if (cert_interface.common_name.has_value()) {
    const auto names = get_asterisk_forms(cert_interface.common_name.value());
    std::copy(names.begin(), names.end(), std::inserter(keys, keys.end()));
  }
  for (const auto& san : cert_interface.sans) {
    const auto names = get_asterisk_forms(san);
    std::copy(names.begin(), names.end(), std::inserter(keys, keys.end()));
  }

  // Could either just check common name, or check for any overlap between SANs.
  // It's more time and space efficient to just check the common name.
  const auto it = std::find_if(cert_map_.begin(), cert_map_.end(), [&keys](const auto& pair) {
    // return !is_disjoint(pair.second.sans, keys);
    return std::find(keys.begin(), keys.end(), pair.first) != keys.end();
  });

  // Certificate already exists.
  if (it != cert_map_.end()) {
    return it->second;
  }

  // Certificate does not exist.
  return std::nullopt;
}

result<memory_certificate> server_store::create_certificate(const certificate_interface& cert_interface) {
  ASSIGN_OR_RETURN(certificate cert, generate_certificate(cert_interface));
  memory_certificate new_cert = {std::move(cert), pkey_, ca_cert_file_fullpath_,
                                 /* sans */};
  return insert(cert_interface.common_name.value_or(""), std::move(new_cert));
}

certificate::serial_t server_store::generate_serial() {
  // TODO: This does not REALLY guarantee that serial numbers are unique for the CA.
  static std::random_device seed;
  static thread_local std::mt19937 generator(seed());
  std::uniform_int_distribution<certificate::serial_t> distribution(std::numeric_limits<certificate::serial_t>::min(),
                                                                    std::numeric_limits<certificate::serial_t>::max());
  return distribution(generator);
}

result<certificate> server_store::generate_certificate(const certificate_interface& cert_interface) {
  certificate cert(openssl::ptrs::in_place);

  if (!ASN1_INTEGER_set(X509_get_serialNumber(*cert), generate_serial())) {
    return error::tls::certificate_creation_error("Error setting certificate serial number.");
  }

  if (!X509_gmtime_adj(X509_get_notBefore(*cert), 0)) {
    return error::tls::certificate_creation_error("Error setting certificate's notBefore property.");
  }

  if (!X509_gmtime_adj(X509_get_notAfter(*cert), default_expiry_time)) {
    return error::tls::certificate_creation_error("Error setting certificate's notAfter property.");
  }

  X509_NAME* cacert_name = X509_get_subject_name(*default_cert_);
  if (!X509_set_issuer_name(*cert, cacert_name)) {
    return error::tls::certificate_creation_error("Error setting certificate's issuer.");
  }

  X509_NAME* name = X509_get_subject_name(*cert);

  bool has_valid_cn = false;
  if (cert_interface.common_name.has_value()) {
    const std::string& cn = cert_interface.common_name.value();
    if (cn.size() < 64) {
      if (!X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char*>(cn.data()), -1,
                                      -1, 0)) {
        return error::tls::certificate_creation_error("Error setting certificate's common name property.");
      }
      has_valid_cn = true;
    }
  }

  if (cert_interface.organization.has_value()) {
    if (!X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC,
                                    reinterpret_cast<const unsigned char*>(cert_interface.organization.value().data()),
                                    -1, -1, 0)) {
      return error::tls::certificate_creation_error("Error setting certificate's organization property.");
    }
  }

  // Set subject alternative names (SANs).
  if (!cert_interface.sans.empty()) {
    if (!X509_set_version(*cert, 2)) {
      return error::tls::certificate_creation_error("Error setting certificate version.");
    }

    std::string subject_alt_name;
    boost::system::error_code error;
    bool first = true;
    for (const auto& san : cert_interface.sans) {
      boost::asio::ip::address::from_string(san, error);

      // Add comma between entries.
      if (first) {
        first = false;
      } else {
        subject_alt_name += ',';
      }

      subject_alt_name += (error == boost::system::errc::success ? "IP:" : "DNS:") + san;
    }

    // No CN => SANs are critical.
    if (!has_valid_cn) {
      subject_alt_name = "critical," + subject_alt_name;
    }

    RETURN_IF_ERROR(add_cert_extension(cert, NID_subject_alt_name, subject_alt_name));
  }

  RETURN_IF_ERROR(add_cert_extension(cert, NID_ext_key_usage, "serverAuth,clientAuth"));

  EVP_PKEY* pub_key = X509_get_pubkey(*default_cert_);
  if (pub_key == nullptr) {
    return error::tls::certificate_creation_error("Error retrieving server certificate's public key.");
  }
  if (!X509_set_pubkey(*cert, pub_key)) {
    return error::tls::certificate_creation_error("Error setting certificate's public key.");
  }

  if (!X509_sign(*cert, *pkey_, EVP_sha256())) {
    return error::tls::certificate_creation_error("Error signing certificate.");
  }

  return cert;
}

}  // namespace proxy::tls::x509
