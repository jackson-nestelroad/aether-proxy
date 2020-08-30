/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "server_store.hpp"

namespace proxy::tcp::tls::x509 {
    const boost::filesystem::path server_store::default_dir
        = (boost::filesystem::path(AETHER_HOME) / "cert_store").make_preferred();

    const boost::filesystem::path server_store::default_properties_file = (default_dir / "proxy.properties").make_preferred();

    server_store::server_store() 
        : pkey(nullptr),
        default_cert(nullptr) 
    {
        // Get properties
        props.parse_file(program::options::instance().ssl_cert_store_properties);

        const auto &dir = program::options::instance().ssl_cert_store_dir;
        auto ca_pkey_path = boost::filesystem::path(dir) / ca_pkey_file_name.data();
        auto ca_cert_path = boost::filesystem::path(dir) / ca_cert_file_name.data();

        // Both files exist, use them
        if (boost::filesystem::exists(ca_pkey_path) && boost::filesystem::exists(ca_cert_path)) {
            // TODO
            // Open file
            // Load as certificate and key pair
        }
        // Need to generate and save the SSL data
        else {
            create_store(dir);
        }
    }

    server_store::~server_store() {
        if (pkey) {
            EVP_PKEY_free(pkey);
        }
        if (default_cert) {
            X509_free(default_cert);
        }
    }

    void server_store::create_store(const std::string &dir) {
        if (!boost::filesystem::exists(dir)) {
            boost::filesystem::create_directory(dir);
        }

        // Generate private key and certificate
        create_ca();

        // Get password from properties
        const auto &password_prop = props.get("password");
        const unsigned char *password = nullptr;
        int password_len = 0;
        if (password_prop.has_value()) {
            password = reinterpret_cast<const unsigned char *>(password_prop.value().data());
            password_len = static_cast<int>(password_prop.value().length());
        }

        auto ca_pkey_path = boost::filesystem::path(dir) / ca_pkey_file_name.data();
        auto ca_cert_path = boost::filesystem::path(dir) / ca_cert_file_name.data();

        // Write private key to disk
        FILE *ca_pkey_file = nullptr;
        auto err = fopen_s(&ca_pkey_file, ca_pkey_path.string().data(), "wb");
        if (err != 0 && ca_pkey_file != nullptr) {
            throw error::tls::ssl_server_store_creation_exception { out::string::stream("Could not open ", ca_pkey_path.string(), " for writing.") };
        }
        if (!PEM_write_PrivateKey(ca_pkey_file, pkey, EVP_des_ede3_cbc(), password, password_len, nullptr, nullptr)) {
            throw error::tls::ssl_server_store_creation_exception { "Failed to write private key to disk." };
        }
        std::fclose(ca_pkey_file);

        // Write certificate to disk
        FILE *ca_cert_file = nullptr;
        err = fopen_s(&ca_cert_file, ca_cert_path.string().data(), "wb");
        if (err != 0 && ca_cert_file != nullptr) {
            throw error::tls::ssl_server_store_creation_exception { out::string::stream("Could not open ", ca_cert_path.string(), " for writing.") };
        }
        if (!PEM_write_X509(ca_cert_file, default_cert)) {
            throw error::tls::ssl_server_store_creation_exception { "Failed to write certificate to disk." };
        }
        std::fclose(ca_cert_file);
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
        EVP_PKEY *pkey = EVP_PKEY_new();
        RSA *rsa_key = RSA_new();

        BIGNUM *big_num = BN_new();
        if (!BN_set_word(big_num, RSA_F4)) {
            BN_free(big_num);
            throw error::tls::ssl_server_store_creation_exception { "Error when setting Big Num for RSA keys." };
        }

        success = RSA_generate_key_ex(rsa_key, key_size, big_num, nullptr);
        BN_free(big_num);
        if (!success) {
            throw error::tls::ssl_server_store_creation_exception { "Error when generating RSA keys." };
        }

        if (!EVP_PKEY_assign_RSA(pkey, rsa_key)) {
            throw error::tls::ssl_server_store_creation_exception { "Error when assigning RSA keys to the public key structure." };
        }

        X509 *cert = X509_new();
        
        if (!X509_set_version(cert, 2)) {
            throw error::tls::ssl_server_store_creation_exception { "Error setting certificate version." };
        }

        if (!ASN1_INTEGER_set(X509_get_serialNumber(cert), static_cast<long>(std::time(nullptr)))) {
            throw error::tls::ssl_server_store_creation_exception { "Error setting certificate serial number." };
        }

        if (!X509_gmtime_adj(X509_get_notBefore(cert), 0)) {
            throw error::tls::ssl_server_store_creation_exception { "Error setting certificate's notBefore property." };
        }

        if (!X509_gmtime_adj(X509_get_notAfter(cert), default_expiry_time)) {
            throw error::tls::ssl_server_store_creation_exception { "Error setting certificate's notAfter property." };
        }

        X509_NAME *name = X509_get_subject_name(cert);

        add_cert_name_entry(name, "C", "country");
        add_cert_name_entry(name, "ST", "state");
        add_cert_name_entry(name, "L", "locality");
        add_cert_name_entry(name, "O", "organization", proxy::constants::lowercase_name);

        if (!X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, 
            reinterpret_cast<const unsigned char *>(proxy::constants::lowercase_name.data()), -1, -1, 0)) {
            throw error::tls::ssl_server_store_creation_exception { "Error setting certificate's common name property." };
        }

        // Self-signed
        if (!X509_set_issuer_name(cert, name)) {
            throw error::tls::ssl_server_store_creation_exception { "Error setting certificate's issuer." };
        }

        if (!X509_set_pubkey(cert, pkey)) {
            throw error::tls::ssl_server_store_creation_exception { "Error setting certificate public key." };
        }

        add_cert_extension(cert, NID_basic_constraints, "critical,CA:TRUE");
        add_cert_extension(cert, NID_key_usage, "critical,keyCertSign,cRLSign");
        add_cert_extension(cert, NID_subject_key_identifier, "hash");
        add_cert_extension(cert, NID_netscape_cert_type, "sslCA");

        if (!X509_sign(cert, pkey, EVP_sha256())) {
            throw error::tls::ssl_server_store_creation_exception { "Error signing certificate." };
        }

        this->pkey = pkey;
        this->default_cert = cert;
    }

    void server_store::add_cert_name_entry(X509_NAME *name, std::string_view entry_code, std::string_view prop_name) {
        const auto &str = prop_name.data();
        const auto &prop = props.get(str);
        if (prop.has_value()) {
            if (!X509_NAME_add_entry_by_txt(name, entry_code.data(), MBSTRING_ASC, 
                reinterpret_cast<const unsigned char *>(prop.value().data()), -1, -1, 0)) {
                throw error::tls::ssl_server_store_creation_exception { out::string::stream("Error setting certificate's ", str, " property.") };
            }
        }
    }

    void server_store::add_cert_name_entry(X509_NAME *name, std::string_view entry_code,
        std::string_view prop_name, std::string_view default_value) {
        const auto &str = prop_name.data();
        if (!X509_NAME_add_entry_by_txt(name, entry_code.data(), MBSTRING_ASC, 
            reinterpret_cast<unsigned char *>(props.get(str).value_or(default_value.data()).data()), -1, -1, 0)) {
            throw error::tls::ssl_server_store_creation_exception { out::string::stream("Error setting certificate's ", str, " property.") };
        }
    }

    void server_store::add_cert_extension(X509 *cert, int ext_id, std::string_view value) {
        X509_EXTENSION *ext = nullptr;
        X509V3_CTX ctx;

        X509V3_set_ctx_nodb(&ctx);
        /* Issuer and subject certs: both the target since it is self signed,
         * no request and no CRL
         */
        X509V3_set_ctx(&ctx, cert, cert, nullptr, nullptr, 0);
        ext = X509V3_EXT_conf_nid(nullptr, &ctx, ext_id, value.data());

        if (!X509_add_ext(cert, ext, -1)) {
            throw error::tls::ssl_server_store_creation_exception { "Error adding certificate extension." };
        }

        X509_EXTENSION_free(ext);
    }
}