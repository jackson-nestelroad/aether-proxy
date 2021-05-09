/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "server_store.hpp"
#include <aether/proxy/server_components.hpp>

namespace proxy::tcp::tls::x509 {
    const boost::filesystem::path server_store::default_dir
        = (boost::filesystem::path(AETHER_HOME) / "cert_store").make_preferred();

    const boost::filesystem::path server_store::default_properties_file = (default_dir / "proxy.properties").make_preferred();

    const boost::filesystem::path server_store::default_dhparam_file = (default_dir / "dhparam.default.pem").make_preferred();

    server_store::server_store(server_components &components) 
        : options(components.options),
        pkey(nullptr),
        default_cert(nullptr),
        dhparams(nullptr)
    {
        // Get properties
        props.parse_file(options.ssl_cert_store_properties);

        const auto &dir = options.ssl_cert_store_dir;
        auto ca_pkey_path = boost::filesystem::path(dir) / ca_pkey_file_name.data();
        auto ca_cert_path = boost::filesystem::path(dir) / ca_cert_file_name.data();

        ca_pkey_file_fullpath = ca_pkey_path.string();
        ca_cert_file_fullpath = ca_cert_path.string();

        // Both files exist, use them
        if (boost::filesystem::exists(ca_pkey_path) && boost::filesystem::exists(ca_cert_path)) {
            read_store(ca_pkey_path.string(), ca_cert_path.string());
        }
        // Need to generate and save the SSL data
        else {
            create_store(dir);
        }

        load_dhparams();
    }

    void server_store::read_store(const std::string &pkey_path, const std::string &cert_path) {
        openssl::ptrs::evp_pkey pkey = nullptr;
        certificate cert = nullptr;

        // Get password from properties
        auto password_prop = props.get("password");
        char *password = nullptr;
        if (password_prop.has_value()) {
            password = const_cast<char *>(password_prop.value().data());
        }

        // Read private key
        openssl::ptrs::unique_native_file ca_pkey_file = nullptr;
        if (ca_pkey_file.open(pkey_path.data(), "rb") || *ca_pkey_file == nullptr) {
            throw error::tls::ssl_server_store_creation_error_exception { out::string::stream("Could not open ", pkey_path, " for reading.") };
        }
        pkey = PEM_read_PrivateKey(*ca_pkey_file, nullptr, nullptr, password);
        if (!pkey) {
            throw error::tls::ssl_server_store_creation_error_exception { "Failed to read existing private key." };
        }

        // Read server certificate
        openssl::ptrs::unique_native_file ca_cert_file = nullptr;
        if (ca_cert_file.open(cert_path.data(), "rb") || *ca_cert_file == nullptr) {
            throw error::tls::ssl_server_store_creation_error_exception { out::string::stream("Could not open ", cert_path, " for reading.") };
        }
        cert = PEM_read_X509(*ca_cert_file, nullptr, nullptr, password);
        if (!cert) {
            throw error::tls::ssl_server_store_creation_error_exception { "Failed to read existing certificate file." };
        }

        this->pkey = pkey;
        this->default_cert = cert;
    }

    void server_store::create_store(const std::string &dir) {
        if (!boost::filesystem::exists(dir)) {
            boost::filesystem::create_directory(dir);
        }

        // Generate private key and certificate
        create_ca();

        // Get password from properties
        auto password_prop = props.get("password");
        const unsigned char *password = nullptr;
        int password_len = 0;
        if (password_prop.has_value()) {
            password = reinterpret_cast<const unsigned char *>(password_prop.value().data());
            password_len = static_cast<int>(password_prop.value().length());
        }

        int error = 0;

        // Write private key to disk
        openssl::ptrs::unique_native_file ca_pkey_file = nullptr;
        if (ca_pkey_file.open(ca_pkey_file_fullpath.data(), "wb") || *ca_pkey_file == nullptr) {
            throw error::tls::ssl_server_store_creation_error_exception { out::string::stream("Could not open ", ca_pkey_file_fullpath, " for writing.") };
        }
        error = PEM_write_PrivateKey(*ca_pkey_file, *pkey, EVP_des_ede3_cbc(), password, password_len, nullptr, nullptr);
        if (!error) {
            throw error::tls::ssl_server_store_creation_error_exception { "Failed to write private key to disk." };
        }

        // Write certificate to disk
        openssl::ptrs::unique_native_file ca_cert_file = nullptr;
        if (ca_cert_file.open(ca_cert_file_fullpath.data(), "wb") || *ca_cert_file == nullptr) {
            throw error::tls::ssl_server_store_creation_error_exception { out::string::stream("Could not open ", ca_pkey_file_fullpath, " for writing.") };
        }
        error = PEM_write_X509(*ca_cert_file, *default_cert);
        if (!error) {
            throw error::tls::ssl_server_store_creation_error_exception { "Failed to write certificate to disk." };
        }
    }

    void server_store::create_ca() {
        const auto &key_size_str = props.get("key_size");
        int key_size = default_key_size;
        if (key_size_str.has_value()) {
            try {
                key_size = boost::lexical_cast<int>(key_size_str.value());
            }
            catch (boost::bad_lexical_cast &) {
                // Do nothing
            }
        }

        int success = 0;

        // TODO: Initialize as nullptr?
        openssl::ptrs::evp_pkey pkey;
        openssl::ptrs::rsa rsa_key;

        openssl::ptrs::bignum big_num;
        if (!BN_set_word(*big_num, RSA_F4)) {
            throw error::tls::ssl_server_store_creation_error_exception { "Error when setting Big Num for RSA keys." };
        }

        success = RSA_generate_key_ex(*rsa_key, key_size, *big_num, nullptr);
        if (!success) {
            throw error::tls::ssl_server_store_creation_error_exception { "Error when generating RSA keys." };
        }

        if (!EVP_PKEY_assign_RSA(*pkey, *rsa_key)) {
            throw error::tls::ssl_server_store_creation_error_exception { "Error when assigning RSA keys to the public key structure." };
        }

        // RSA instances are automatically freed by their EVP_PKEY owners, so increment to avoid deallocation
        rsa_key.increment();

        certificate cert;
        
        if (!X509_set_version(*cert, 2)) {
            throw error::tls::ssl_server_store_creation_error_exception { "Error setting certificate version." };
        }

        if (!ASN1_INTEGER_set(X509_get_serialNumber(*cert), generate_serial())) {
            throw error::tls::ssl_server_store_creation_error_exception { "Error setting certificate serial number." };
        }

        if (!X509_gmtime_adj(X509_get_notBefore(*cert), 0)) {
            throw error::tls::ssl_server_store_creation_error_exception { "Error setting certificate's notBefore property." };
        }

        if (!X509_gmtime_adj(X509_get_notAfter(*cert), default_expiry_time)) {
            throw error::tls::ssl_server_store_creation_error_exception { "Error setting certificate's notAfter property." };
        }

        // Attach properties to certificate
        X509_NAME *name = X509_get_subject_name(*cert);
        add_cert_name_entry_from_props(name, NID_commonName, "name", proxy::constants::lowercase_name.data());
        add_cert_name_entry_from_props(name, NID_countryName, "country");
        add_cert_name_entry_from_props(name, NID_stateOrProvinceName, "state");
        add_cert_name_entry_from_props(name, NID_localityName, "locality");
        add_cert_name_entry_from_props(name, NID_organizationName, "organization");
        add_cert_name_entry_from_props(name, NID_organizationalUnitName, "organizational_unit");
        add_cert_name_entry_from_props(name, NID_dnQualifier, "dn_qualifier");

        // Self-signed
        if (!X509_set_issuer_name(*cert, name)) {
            throw error::tls::ssl_server_store_creation_error_exception { "Error setting certificate's issuer." };
        }

        if (!X509_set_pubkey(*cert, *pkey)) {
            throw error::tls::ssl_server_store_creation_error_exception { "Error setting certificate public key." };
        }

        add_cert_extension(cert, NID_basic_constraints, "critical,CA:TRUE");
        add_cert_extension(cert, NID_netscape_cert_type, "sslCA");
        add_cert_extension(cert, NID_ext_key_usage, "serverAuth,clientAuth,emailProtection,timeStamping,msCodeInd,msCodeCom,msCTLSign,msSGC,msEFS,nsSGC");
        add_cert_extension(cert, NID_key_usage, "critical,keyCertSign,cRLSign");
        add_cert_extension(cert, NID_subject_key_identifier, "hash");

        if (!X509_sign(*cert, *pkey, EVP_sha256())) {
            throw error::tls::ssl_server_store_creation_error_exception { "Error signing certificate." };
        }

        this->pkey = pkey;
        this->default_cert = cert;
    }

    void server_store::add_cert_name_entry_from_props(X509_NAME *name, int entry_code, std::string_view prop_name) {
        const auto &str = prop_name.data();
        const auto &prop = props.get(str);
        if (prop.has_value()) {
            if (!X509_NAME_add_entry_by_NID(name, entry_code, MBSTRING_ASC, 
                reinterpret_cast<const unsigned char *>(prop.value().data()), -1, -1, 0)) {
                throw error::tls::ssl_server_store_creation_error_exception { out::string::stream("Error setting certificate's ", str, " property.") };
            }
        }
    }

    void server_store::add_cert_name_entry_from_props(X509_NAME *name, int entry_code,
        std::string_view prop_name, std::string_view default_value) {
        const auto &str = prop_name.data();
        if (!X509_NAME_add_entry_by_NID(name, entry_code, MBSTRING_ASC, 
            reinterpret_cast<const unsigned char *>(props.get(str).value_or(default_value.data()).data()), -1, -1, 0)) {
            throw error::tls::ssl_server_store_creation_error_exception { out::string::stream("Error setting certificate's ", str, " property.") };
        }
    }

    void server_store::add_cert_extension(certificate cert, int ext_id, std::string_view value) {
        X509V3_CTX ctx { };
        X509V3_set_ctx_nodb(&ctx);
        X509V3_set_ctx(&ctx, *cert, *cert, nullptr, nullptr, 0);

        openssl::ptrs::x509_extension ext = X509V3_EXT_conf_nid(nullptr, &ctx, ext_id, value.data());

        if (!X509_add_ext(*cert, *ext, -1)) {
            throw error::tls::ssl_server_store_creation_error_exception { "Error adding certificate extension." };
        }
    }

    void server_store::load_dhparams() {
        const auto &dhparam_file_name = options.ssl_dhparam_file;

        // We need this file to load the dhparams
        // It is much too slow to generate this in the program itself, so the program should be run with the parameters already generated
        // This file can be generated using "openssl dhparam"
        if (!boost::filesystem::exists(dhparam_file_name)) {
            // Default file was not found
            if (dhparam_file_name == default_dhparam_file) {
                throw error::tls::ssl_server_store_creation_error_exception { "The server's Diffie-Hellman parameters file was not found. This is a fatal error, and it is likely a result of the program not being set up correctly." };
            }
            else {
                throw error::tls::ssl_server_store_creation_error_exception { "Could not find Diffie-Hellman parameters file at " + dhparam_file_name };
            }
        }

        openssl::ptrs::unique_native_file dhparam_file = nullptr;
        if (dhparam_file.open(dhparam_file_name.data(), "rb") || *dhparam_file == nullptr) {
            throw error::tls::ssl_server_store_creation_error_exception { out::string::stream("Failed to open ", dhparam_file_name, " for reading.") };
        }

        openssl::ptrs::dh dh = nullptr;
        dh = PEM_read_DHparams(*dhparam_file, nullptr, nullptr, nullptr);
        if (!dh) {
            throw error::tls::ssl_server_store_creation_error_exception { "Failed to read Diffie-Hellman parameters from disk." };
        }
        this->dhparams = std::move(dh);
    }

    void server_store::insert(const std::string &key, const memory_certificate &cert) {
        std::lock_guard<std::mutex> lock(cert_data_mutex);
        cert_map.emplace(key, cert);
        cert_queue.push(key);
        if (cert_map.size() > max_num_certs) {
            cert_map.erase(cert_queue.front());
            cert_queue.pop();
        }
    }

    std::vector<std::string> server_store::get_asterisk_forms(const std::string &domain) {
        std::vector<std::string> asterisk_forms { domain };
        std::size_t prev = 0;
        std::size_t pos = 0;
        while ((pos = domain.find('.', prev)) != std::string::npos) {
            prev = pos + 1;
            asterisk_forms.push_back("*." + domain.substr(prev));
        }
        return asterisk_forms;
    }

    openssl::ptrs::dh &server_store::get_dhparams() {
        return dhparams;
    }

    template <typename Set1, typename Set2>
    bool is_disjoint(const Set1 &set1, const Set2 &set2) {
        if (set1.empty() || set2.empty()) {
            return true;
        }

        typename Set1::const_iterator
            it1 = set1.begin(),
            it1_end = set1.end();
        typename Set2::const_iterator
            it2 = set2.begin(),
            it2_end = set2.end();

        if (*it1 > *set2.rbegin() || *it2 > *set1.rbegin()) {
            return true;
        }

        while (it1 != it1_end && it2 != it2_end) {
            if (*it1 == *it2) {
                return false;
            }
            if (*it1 < *it2) {
                ++it1;
            }
            else {
                ++it2;
            }
        }

        return true;
    }

    std::optional<memory_certificate> server_store::get_certificate(const certificate_interface &cert_interface) {
        std::set<std::string> keys;
        if (cert_interface.common_name.has_value()) {
            const auto &&names = get_asterisk_forms(cert_interface.common_name.value());
            std::copy(names.begin(), names.end(), std::inserter(keys, keys.end()));
        }
        for (const auto &san : cert_interface.sans) {
            const auto &&names = get_asterisk_forms(san);
            std::copy(names.begin(), names.end(), std::inserter(keys, keys.end()));
        }

        // Could either just check common name, or check for any overlap between SANS
        // It's more time and space efficient to just check the common name
        const auto &&it = std::find_if(cert_map.begin(), cert_map.end(), [&keys](const auto &pair) {
            // return !is_disjoint(pair.second.sans, keys);
            return std::find(keys.begin(), keys.end(), pair.first) != keys.end();
        });
        
        // Certificate already exists
        if (it != cert_map.end()) {
            return it->second;
        }

        // Certificate does not exist
        return std::nullopt;
    }

    memory_certificate server_store::create_certificate(const certificate_interface &cert_interface) {
        memory_certificate new_cert = { generate_certificate(cert_interface), pkey, ca_cert_file_fullpath.data(), /* sans */ };
        insert(cert_interface.common_name.value_or(""), new_cert);
        return new_cert;
    }

    certificate::serial_t server_store::generate_serial() {
        // TODO: This does not REALLY guarantee that serial numbers are unique for the CA
        static std::random_device seed;
        static thread_local std::mt19937 generator(seed());
        std::uniform_int_distribution<certificate::serial_t> distribution(
            std::numeric_limits<certificate::serial_t>::min(),
            std::numeric_limits<certificate::serial_t>::max()
        );
        return distribution(generator);
    }

    certificate server_store::generate_certificate(const certificate_interface &cert_interface) {
        certificate cert;

        if (!ASN1_INTEGER_set(X509_get_serialNumber(*cert), generate_serial())) {
            throw error::tls::certificate_creation_error_exception { "Error setting certificate serial number." };
        }

        if (!X509_gmtime_adj(X509_get_notBefore(*cert), 0)) {
            throw error::tls::certificate_creation_error_exception { "Error setting certificate's notBefore property." };
        }

        if (!X509_gmtime_adj(X509_get_notAfter(*cert), default_expiry_time)) {
            throw error::tls::certificate_creation_error_exception { "Error setting certificate's notAfter property." };
        }

        X509_NAME *cacert_name = X509_get_subject_name(*default_cert);
        if (!X509_set_issuer_name(*cert, cacert_name)) {
            throw error::tls::certificate_creation_error_exception { "Error setting certificate's issuer." };
        }

        X509_NAME *name = X509_get_subject_name(*cert);

        bool has_valid_cn = false;
        if (cert_interface.common_name.has_value()) {
            const std::string &cn = cert_interface.common_name.value();
            if (cn.size() < 64) {
                if (!X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                    reinterpret_cast<const unsigned char *>(cn.data()), -1, -1, 0)) {
                    throw error::tls::certificate_creation_error_exception { "Error setting certificate's common name property." };
                }
                has_valid_cn = true;
            }
        }

        if (cert_interface.organization.has_value()) {
            if (!X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC,
                reinterpret_cast<const unsigned char *>(cert_interface.organization.value().data()), -1, -1, 0)) {
                throw error::tls::certificate_creation_error_exception { "Error setting certificate's organization property." };
            }
        }

        // Set subject alternative names (SANs)
        if (!cert_interface.sans.empty()) {
            if (!X509_set_version(*cert, 2)) {
                throw error::tls::certificate_creation_error_exception { "Error setting certificate version." };
            }

            std::string subject_alt_name;
            boost::system::error_code error;
            bool first = true;
            for (const auto &san : cert_interface.sans) {
                boost::asio::ip::address::from_string(san, error);

                // Add comma between entries
                if (first) {
                    first = false;
                }
                else {
                    subject_alt_name += ',';
                }

                subject_alt_name += (error == boost::system::errc::success ? "IP:" : "DNS:") + san;
            }

            // No CN => SANs are critical
            if (!has_valid_cn) {
                subject_alt_name = "critical," + subject_alt_name;
            }

            add_cert_extension(cert, NID_subject_alt_name, subject_alt_name);
        }

        add_cert_extension(cert, NID_ext_key_usage, "serverAuth,clientAuth");

        EVP_PKEY *pub_key = X509_get_pubkey(*default_cert);
        if (pub_key == nullptr) {
            throw error::tls::certificate_creation_error_exception { "Error retrieving server certificate's public key." };
        }
        if (!X509_set_pubkey(*cert, pub_key)) {
            throw error::tls::certificate_creation_error_exception { "Error setting certificate's public key." };
        }

        if (!X509_sign(*cert, *pkey, EVP_sha256())) {
            throw error::tls::certificate_creation_error_exception { "Error signing certificate." };
        }

        return cert;
    }
}