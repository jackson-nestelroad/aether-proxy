/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "certificate.hpp"

namespace proxy::tcp::tls::x509 {
    certificate::certificate(SSL *ssl)
        : openssl::ptrs::x509(nullptr) 
    {
        native = SSL_get_peer_certificate(ssl);
    }

    std::optional<std::string> certificate::get_nid_from_name(int nid) {
        X509_NAME *name = X509_get_subject_name(native);
        if (!name) {
            throw error::tls::certificate_issuer_not_found_exception { };
        }

        int index = X509_NAME_get_index_by_NID(name, nid, -1);

        // This entry does not exist
        if (index < 0) {
            return std::nullopt;
        }

        X509_NAME_ENTRY *common_name_entry = X509_NAME_get_entry(name, index);
        if (!common_name_entry) {
            throw error::tls::certificate_name_entry_error_exception { };
        }

        ASN1_STRING *common_name = X509_NAME_ENTRY_get_data(common_name_entry);
        if (!common_name) {
            throw error::tls::certificate_name_entry_error_exception { };
        }

        unsigned char *str = nullptr;
        int asn1_length = ASN1_STRING_to_UTF8(&str, common_name);
        if (!str || asn1_length == 0) {
            throw error::tls::certificate_name_entry_error_exception { };
        }

        auto result = std::string(str, str + asn1_length);
        OPENSSL_free(str);
        return result;
    }

    std::optional<std::string> certificate::common_name() {
        return get_nid_from_name(NID_commonName);
    }

    std::optional<std::string> certificate::organization() {
        return get_nid_from_name(NID_organizationName);
    }

    std::vector<std::string> certificate::sans() {
        std::vector<std::string> names;
        openssl::ptrs::general_names names_ext = static_cast<GENERAL_NAMES *>(X509_get_ext_d2i(native, NID_subject_alt_name, nullptr, nullptr));
        if (!names_ext) {
            return names;
        }

        int num_names = sk_GENERAL_NAME_num(*names_ext);
        for (int i = 0; i < num_names; ++i) {
            auto entry = sk_GENERAL_NAME_value(*names_ext, i);
            // Ignore null entries or non-DNS entries
            if (!entry || entry->type != GEN_DNS) {
                continue;
            }

            unsigned char *str = nullptr;
            int asn1_length = ASN1_STRING_to_UTF8(&str, entry->d.dNSName);
            // TODO: May check UTF8 length using std::strlen and make sure the two lenghts are equal

            if (str && asn1_length != 0) {
                names.push_back(std::string(str, str + asn1_length));
            }

            OPENSSL_free(str);
        }

        return names;
    }
}