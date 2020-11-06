/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <queue>
#include <boost/filesystem.hpp>
#include <boost/asio/ssl.hpp>

#include <aether/program/options.hpp>
#include <aether/program/properties.hpp>
#include <aether/proxy/error/exceptions.hpp>
#include <aether/proxy/constants/server_constants.hpp>
#include <aether/proxy/tcp/tls/x509/certificate.hpp>
#include <aether/proxy/tcp/tls/x509/memory_certificate.hpp>
#include <aether/proxy/tcp/tls/openssl/smart_ptrs.hpp>

namespace proxy::tcp::tls::x509 {
    /*
        Class for a X.509 certificate store to be used by the SSL server.
        Responsible for SSL certificate generation and storage.
    */
    class server_store {
    private:
        static constexpr int default_key_size = 2048;
        static constexpr long default_expiry_time = 60 * 60 * 24 * 365 * 3;
        static constexpr std::size_t max_num_certs = 100;

        static constexpr std::string_view ca_pkey_file_suffix = "-cakey.pem";
        static constexpr std::string_view ca_cert_file_suffix = "-cacert.pem";

        static constexpr std::string_view ca_pkey_file_name = util::string_view::join_v<proxy::constants::lowercase_name, ca_pkey_file_suffix>;
        static constexpr std::string_view ca_cert_file_name = util::string_view::join_v<proxy::constants::lowercase_name, ca_cert_file_suffix>;

        program::properties props;

        std::string ca_pkey_file_fullpath;
        std::string ca_cert_file_fullpath;

        openssl::ptrs::evp_pkey pkey;
        certificate default_cert;
        openssl::ptrs::dh dhparams;

        // Maps a common name or domain name to an in-memory certificate
        std::map<std::string, memory_certificate> cert_map;

        // The top of the queue is the next certificate to delete when the max capacity is reached
        std::queue<std::string> cert_queue;

        void read_store(const std::string &pkey_path, const std::string &cert_path);

        void create_store(const std::string &dir);
        void create_ca();
        void add_cert_name_entry_from_props(X509_NAME *name, std::string_view entry_code, std::string_view prop_name);
        void add_cert_name_entry_from_props(X509_NAME *name, std::string_view entry_code, std::string_view prop_name, std::string_view default_value);
        void add_cert_extension(certificate cert, int ext_id, std::string_view value);

        void load_dhparams();

        void insert(const std::string &key, const memory_certificate &cert);
        std::vector<std::string> get_asterisk_forms(const std::string &domain);
        certificate create_certificate(const std::optional<std::string> &common_name, const std::set<std::string> &sans, const std::optional<std::string> &organization);

    public:
        static const boost::filesystem::path default_dir;
        static const boost::filesystem::path default_properties_file;
        static const boost::filesystem::path default_dhparam_file;

        server_store();

        openssl::ptrs::dh &get_dhparams();
        memory_certificate get_certificate(const std::optional<std::string> &common_name, const std::set<std::string> &sans, const std::optional<std::string> &organization);
    };
}