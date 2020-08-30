/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/filesystem.hpp>
#include <boost/asio/ssl.hpp>

#include <aether/program/options.hpp>
#include <aether/program/properties.hpp>
#include <aether/proxy/error/exceptions.hpp>
#include <aether/proxy/constants/server_constants.hpp>

namespace proxy::tcp::tls::x509 {
    class server_store {
    private:
        static constexpr int default_key_size = 2048;
        static constexpr long default_expiry_time = 60 * 60 * 24 * 365 * 3;
        static constexpr std::string_view ca_pkey_file_suffix = "-cakey.pem";
        static constexpr std::string_view ca_cert_file_suffix = "-cacert.pem";

        static constexpr std::string_view ca_pkey_file_name = util::string_view::join_v<proxy::constants::lowercase_name, ca_pkey_file_suffix>;
        static constexpr std::string_view ca_cert_file_name = util::string_view::join_v<proxy::constants::lowercase_name, ca_cert_file_suffix>;

        program::properties props;

        EVP_PKEY *pkey;
        X509 *default_cert;

        void create_store(const std::string &dir);
        void create_ca();
        void add_cert_name_entry(X509_NAME *name, std::string_view entry_code, std::string_view prop_name);
        void add_cert_name_entry(X509_NAME *name, std::string_view entry_code, std::string_view prop_name, std::string_view default_value);
        void add_cert_extension(X509 *cert, int ext_id, std::string_view value);

    public:
        static const boost::filesystem::path default_dir;
        static const boost::filesystem::path default_properties_file;

        server_store();
        ~server_store();
    };
}