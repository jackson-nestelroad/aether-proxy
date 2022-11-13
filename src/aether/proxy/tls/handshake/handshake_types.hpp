/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>

#include "aether/proxy/error/exceptions.hpp"
#include "aether/proxy/types.hpp"

namespace proxy::tls::handshake {
enum class extension_type : double_byte_t {
  server_name = 0x0000,
  max_fragment_length = 0x0001,
  client_certificate_url = 0x0002,
  trusted_ca_keys = 0x0003,
  truncated_hmac = 0x0004,
  status_request = 0x0005,
  user_mapping = 0x0006,
  client_authz = 0x0007,
  server_authz = 0x0008,
  cert_type = 0x0009,
  supported_groups = 0x000A,
  ec_point_formats = 0x000B,
  srp = 0x000C,
  signature_algorithms = 0x000D,
  use_srtp = 0x000E,
  heartbeat = 0x000F,
  application_layer_protocol_negotiation = 0x0010,
  status_request_v2 = 0x0011,
  signed_certificate_timestamp = 0x0012,
  client_certificate_type = 0x0013,
  server_certificate_type = 0x0014,
  padding = 0x0015,
  encrypt_then_mac = 0x0016,
  extended_master_secret = 0x0017,
  token_binding = 0x0018,
  cached_info = 0x0019,
  tls_lts = 0x001A,
  compress_certificate = 0x001B,
  record_size_limit = 0x001C,
  pwd_protect = 0x001D,
  pwd_clear = 0x001E,
  password_salt = 0x001F,
  ticket_pinning = 0x0020,
  tls_cert_with_extern_psk = 0x0021,
  delegated_credentials = 0x0022,
  session_ticket = 0x0023,
  pre_shared_key = 0x0029,
  early_data = 0x002A,
  supported_versions = 0x002B,
  cookie = 0x002C,
  psk_key_exchange_modes = 0x002D,
  certificate_authorities = 0x002F,
  oid_filters = 0x0030,
  post_handshake_auth = 0x0031,
  signature_algorithms_cert = 0x0032,
  key_share = 0x0033,
  transparency_info = 0x0034,
  external_id_hash = 0x0037,
  external_session_id = 0x0038
};

/*
    let seen = new Map();
    let list = [];
    document.querySelectorAll('tbody tr').forEach(tr => {
        let cols = tr.querySelectorAll('td');
        if (cols.length > 5) {
            let name = cols[1].innerText;
            if (name) {
                let enumName = name.replace(/-/g, '_');
                if (enumName === 'NULL') {
                    enumName = '_NULL';
                }
                let val = seen.get(name);
                val = val === undefined ? 0 : val;

                if (val > 0) {
                    enumName += `__${val}`;
                }
                seen.set(name, val + 1);
                let id = cols[0].innerText;
                id = id.substring(1, id.length - 1);
                list.push(`X(${id}, ${enumName}, "${cols[5].innerText}") \\`);
            }
        }
    });
    console.log(list.join('\n'));
*/

// Taken from https://testssl.sh/openssl-iana.mapping.html
#define CIPHER_SUITE_NAMES(X)                                                                       \
  X(0x01, NULL_MD5, "TLS_RSA_WITH_NULL_MD5")                                                        \
  X(0x02, NULL_SHA, "TLS_RSA_WITH_NULL_SHA")                                                        \
  X(0x03, EXP_RC4_MD5, "TLS_RSA_EXPORT_WITH_RC4_40_MD5")                                            \
  X(0x04, RC4_MD5, "TLS_RSA_WITH_RC4_128_MD5")                                                      \
  X(0x05, RC4_SHA, "TLS_RSA_WITH_RC4_128_SHA")                                                      \
  X(0x06, EXP_RC2_CBC_MD5, "TLS_RSA_EXPORT_WITH_RC2_CBC_40_MD5")                                    \
  X(0x07, IDEA_CBC_SHA, "TLS_RSA_WITH_IDEA_CBC_SHA")                                                \
  X(0x08, EXP_DES_CBC_SHA, "TLS_RSA_EXPORT_WITH_DES40_CBC_SHA")                                     \
  X(0x09, DES_CBC_SHA, "TLS_RSA_WITH_DES_CBC_SHA")                                                  \
  X(0x0a, DES_CBC3_SHA, "TLS_RSA_WITH_3DES_EDE_CBC_SHA")                                            \
  X(0x0b, EXP_DH_DSS_DES_CBC_SHA, "TLS_DH_DSS_EXPORT_WITH_DES40_CBC_SHA")                           \
  X(0x0c, DH_DSS_DES_CBC_SHA, "TLS_DH_DSS_WITH_DES_CBC_SHA")                                        \
  X(0x0d, DH_DSS_DES_CBC3_SHA, "TLS_DH_DSS_WITH_3DES_EDE_CBC_SHA")                                  \
  X(0x0e, EXP_DH_RSA_DES_CBC_SHA, "TLS_DH_RSA_EXPORT_WITH_DES40_CBC_SHA")                           \
  X(0x0f, DH_RSA_DES_CBC_SHA, "TLS_DH_RSA_WITH_DES_CBC_SHA")                                        \
  X(0x10, DH_RSA_DES_CBC3_SHA, "TLS_DH_RSA_WITH_3DES_EDE_CBC_SHA")                                  \
  X(0x11, EXP_EDH_DSS_DES_CBC_SHA, "TLS_DHE_DSS_EXPORT_WITH_DES40_CBC_SHA")                         \
  X(0x12, EDH_DSS_DES_CBC_SHA, "TLS_DHE_DSS_WITH_DES_CBC_SHA")                                      \
  X(0x13, EDH_DSS_DES_CBC3_SHA, "TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA")                                \
  X(0x14, EXP_EDH_RSA_DES_CBC_SHA, "TLS_DHE_RSA_EXPORT_WITH_DES40_CBC_SHA")                         \
  X(0x15, EDH_RSA_DES_CBC_SHA, "TLS_DHE_RSA_WITH_DES_CBC_SHA")                                      \
  X(0x16, EDH_RSA_DES_CBC3_SHA, "TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA")                                \
  X(0x17, EXP_ADH_RC4_MD5, "TLS_DH_anon_EXPORT_WITH_RC4_40_MD5")                                    \
  X(0x18, ADH_RC4_MD5, "TLS_DH_anon_WITH_RC4_128_MD5")                                              \
  X(0x19, EXP_ADH_DES_CBC_SHA, "TLS_DH_anon_EXPORT_WITH_DES40_CBC_SHA")                             \
  X(0x1a, ADH_DES_CBC_SHA, "TLS_DH_anon_WITH_DES_CBC_SHA")                                          \
  X(0x1b, ADH_DES_CBC3_SHA, "TLS_DH_anon_WITH_3DES_EDE_CBC_SHA")                                    \
  X(0x1e, KRB5_DES_CBC_SHA, "TLS_KRB5_WITH_DES_CBC_SHA")                                            \
  X(0x1f, KRB5_DES_CBC3_SHA, "TLS_KRB5_WITH_3DES_EDE_CBC_SHA")                                      \
  X(0x20, KRB5_RC4_SHA, "TLS_KRB5_WITH_RC4_128_SHA")                                                \
  X(0x21, KRB5_IDEA_CBC_SHA, "TLS_KRB5_WITH_IDEA_CBC_SHA")                                          \
  X(0x22, KRB5_DES_CBC_MD5, "TLS_KRB5_WITH_DES_CBC_MD5")                                            \
  X(0x23, KRB5_DES_CBC3_MD5, "TLS_KRB5_WITH_3DES_EDE_CBC_MD5")                                      \
  X(0x24, KRB5_RC4_MD5, "TLS_KRB5_WITH_RC4_128_MD5")                                                \
  X(0x25, KRB5_IDEA_CBC_MD5, "TLS_KRB5_WITH_IDEA_CBC_MD5")                                          \
  X(0x26, EXP_KRB5_DES_CBC_SHA, "TLS_KRB5_EXPORT_WITH_DES_CBC_40_SHA")                              \
  X(0x27, EXP_KRB5_RC2_CBC_SHA, "TLS_KRB5_EXPORT_WITH_RC2_CBC_40_SHA")                              \
  X(0x28, EXP_KRB5_RC4_SHA, "TLS_KRB5_EXPORT_WITH_RC4_40_SHA")                                      \
  X(0x29, EXP_KRB5_DES_CBC_MD5, "TLS_KRB5_EXPORT_WITH_DES_CBC_40_MD5")                              \
  X(0x2a, EXP_KRB5_RC2_CBC_MD5, "TLS_KRB5_EXPORT_WITH_RC2_CBC_40_MD5")                              \
  X(0x2b, EXP_KRB5_RC4_MD5, "TLS_KRB5_EXPORT_WITH_RC4_40_MD5")                                      \
  X(0x2c, PSK_NULL_SHA, "TLS_PSK_WITH_NULL_SHA")                                                    \
  X(0x2d, DHE_PSK_NULL_SHA, "TLS_DHE_PSK_WITH_NULL_SHA")                                            \
  X(0x2e, RSA_PSK_NULL_SHA, "TLS_RSA_PSK_WITH_NULL_SHA")                                            \
  X(0x2f, AES128_SHA, "TLS_RSA_WITH_AES_128_CBC_SHA")                                               \
  X(0x30, DH_DSS_AES128_SHA, "TLS_DH_DSS_WITH_AES_128_CBC_SHA")                                     \
  X(0x31, DH_RSA_AES128_SHA, "TLS_DH_RSA_WITH_AES_128_CBC_SHA")                                     \
  X(0x32, DHE_DSS_AES128_SHA, "TLS_DHE_DSS_WITH_AES_128_CBC_SHA")                                   \
  X(0x33, DHE_RSA_AES128_SHA, "TLS_DHE_RSA_WITH_AES_128_CBC_SHA")                                   \
  X(0x34, ADH_AES128_SHA, "TLS_DH_anon_WITH_AES_128_CBC_SHA")                                       \
  X(0x35, AES256_SHA, "TLS_RSA_WITH_AES_256_CBC_SHA")                                               \
  X(0x36, DH_DSS_AES256_SHA, "TLS_DH_DSS_WITH_AES_256_CBC_SHA")                                     \
  X(0x37, DH_RSA_AES256_SHA, "TLS_DH_RSA_WITH_AES_256_CBC_SHA")                                     \
  X(0x38, DHE_DSS_AES256_SHA, "TLS_DHE_DSS_WITH_AES_256_CBC_SHA")                                   \
  X(0x39, DHE_RSA_AES256_SHA, "TLS_DHE_RSA_WITH_AES_256_CBC_SHA")                                   \
  X(0x3a, ADH_AES256_SHA, "TLS_DH_anon_WITH_AES_256_CBC_SHA")                                       \
  X(0x3b, NULL_SHA256, "TLS_RSA_WITH_NULL_SHA256")                                                  \
  X(0x3c, AES128_SHA256, "TLS_RSA_WITH_AES_128_CBC_SHA256")                                         \
  X(0x3d, AES256_SHA256, "TLS_RSA_WITH_AES_256_CBC_SHA256")                                         \
  X(0x3e, DH_DSS_AES128_SHA256, "TLS_DH_DSS_WITH_AES_128_CBC_SHA256")                               \
  X(0x3f, DH_RSA_AES128_SHA256, "TLS_DH_RSA_WITH_AES_128_CBC_SHA256")                               \
  X(0x40, DHE_DSS_AES128_SHA256, "TLS_DHE_DSS_WITH_AES_128_CBC_SHA256")                             \
  X(0x41, CAMELLIA128_SHA, "TLS_RSA_WITH_CAMELLIA_128_CBC_SHA")                                     \
  X(0x42, DH_DSS_CAMELLIA128_SHA, "TLS_DH_DSS_WITH_CAMELLIA_128_CBC_SHA")                           \
  X(0x43, DH_RSA_CAMELLIA128_SHA, "TLS_DH_RSA_WITH_CAMELLIA_128_CBC_SHA")                           \
  X(0x44, DHE_DSS_CAMELLIA128_SHA, "TLS_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA")                         \
  X(0x45, DHE_RSA_CAMELLIA128_SHA, "TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA")                         \
  X(0x46, ADH_CAMELLIA128_SHA, "TLS_DH_anon_WITH_CAMELLIA_128_CBC_SHA")                             \
  X(0x60, EXP1024_RC4_MD5, "TLS_RSA_EXPORT1024_WITH_RC4_56_MD5")                                    \
  X(0x61, EXP1024_RC2_CBC_MD5, "TLS_RSA_EXPORT1024_WITH_RC2_CBC_56_MD5")                            \
  X(0x62, EXP1024_DES_CBC_SHA, "TLS_RSA_EXPORT1024_WITH_DES_CBC_SHA")                               \
  X(0x63, EXP1024_DHE_DSS_DES_CBC_SHA, "TLS_DHE_DSS_EXPORT1024_WITH_DES_CBC_SHA")                   \
  X(0x64, EXP1024_RC4_SHA, "TLS_RSA_EXPORT1024_WITH_RC4_56_SHA")                                    \
  X(0x65, EXP1024_DHE_DSS_RC4_SHA, "TLS_DHE_DSS_EXPORT1024_WITH_RC4_56_SHA")                        \
  X(0x66, DHE_DSS_RC4_SHA, "TLS_DHE_DSS_WITH_RC4_128_SHA")                                          \
  X(0x67, DHE_RSA_AES128_SHA256, "TLS_DHE_RSA_WITH_AES_128_CBC_SHA256")                             \
  X(0x68, DH_DSS_AES256_SHA256, "TLS_DH_DSS_WITH_AES_256_CBC_SHA256")                               \
  X(0x69, DH_RSA_AES256_SHA256, "TLS_DH_RSA_WITH_AES_256_CBC_SHA256")                               \
  X(0x6a, DHE_DSS_AES256_SHA256, "TLS_DHE_DSS_WITH_AES_256_CBC_SHA256")                             \
  X(0x6b, DHE_RSA_AES256_SHA256, "TLS_DHE_RSA_WITH_AES_256_CBC_SHA256")                             \
  X(0x6c, ADH_AES128_SHA256, "TLS_DH_anon_WITH_AES_128_CBC_SHA256")                                 \
  X(0x6d, ADH_AES256_SHA256, "TLS_DH_anon_WITH_AES_256_CBC_SHA256")                                 \
  X(0x80, GOST94_GOST89_GOST89, "TLS_GOSTR341094_WITH_28147_CNT_IMIT")                              \
  X(0x81, GOST2001_GOST89_GOST89, "TLS_GOSTR341001_WITH_28147_CNT_IMIT")                            \
  X(0x82, GOST94_NULL_GOST94, "TLS_GOSTR341001_WITH_NULL_GOSTR3411")                                \
  X(0x83, GOST2001_GOST89_GOST89__1, "TLS_GOSTR341094_WITH_NULL_GOSTR3411")                         \
  X(0x84, CAMELLIA256_SHA, "TLS_RSA_WITH_CAMELLIA_256_CBC_SHA")                                     \
  X(0x85, DH_DSS_CAMELLIA256_SHA, "TLS_DH_DSS_WITH_CAMELLIA_256_CBC_SHA")                           \
  X(0x86, DH_RSA_CAMELLIA256_SHA, "TLS_DH_RSA_WITH_CAMELLIA_256_CBC_SHA")                           \
  X(0x87, DHE_DSS_CAMELLIA256_SHA, "TLS_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA")                         \
  X(0x88, DHE_RSA_CAMELLIA256_SHA, "TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA")                         \
  X(0x89, ADH_CAMELLIA256_SHA, "TLS_DH_anon_WITH_CAMELLIA_256_CBC_SHA")                             \
  X(0x8a, PSK_RC4_SHA, "TLS_PSK_WITH_RC4_128_SHA")                                                  \
  X(0x8b, PSK_3DES_EDE_CBC_SHA, "TLS_PSK_WITH_3DES_EDE_CBC_SHA")                                    \
  X(0x8c, PSK_AES128_CBC_SHA, "TLS_PSK_WITH_AES_128_CBC_SHA")                                       \
  X(0x8d, PSK_AES256_CBC_SHA, "TLS_PSK_WITH_AES_256_CBC_SHA")                                       \
  X(0x96, SEED_SHA, "TLS_RSA_WITH_SEED_CBC_SHA")                                                    \
  X(0x97, DH_DSS_SEED_SHA, "TLS_DH_DSS_WITH_SEED_CBC_SHA")                                          \
  X(0x98, DH_RSA_SEED_SHA, "TLS_DH_RSA_WITH_SEED_CBC_SHA")                                          \
  X(0x99, DHE_DSS_SEED_SHA, "TLS_DHE_DSS_WITH_SEED_CBC_SHA")                                        \
  X(0x9a, DHE_RSA_SEED_SHA, "TLS_DHE_RSA_WITH_SEED_CBC_SHA")                                        \
  X(0x9b, ADH_SEED_SHA, "TLS_DH_anon_WITH_SEED_CBC_SHA")                                            \
  X(0x9c, AES128_GCM_SHA256, "TLS_RSA_WITH_AES_128_GCM_SHA256")                                     \
  X(0x9d, AES256_GCM_SHA384, "TLS_RSA_WITH_AES_256_GCM_SHA384")                                     \
  X(0x9e, DHE_RSA_AES128_GCM_SHA256, "TLS_DHE_RSA_WITH_AES_128_GCM_SHA256")                         \
  X(0x9f, DHE_RSA_AES256_GCM_SHA384, "TLS_DHE_RSA_WITH_AES_256_GCM_SHA384")                         \
  X(0xa0, DH_RSA_AES128_GCM_SHA256, "TLS_DH_RSA_WITH_AES_128_GCM_SHA256")                           \
  X(0xa1, DH_RSA_AES256_GCM_SHA384, "TLS_DH_RSA_WITH_AES_256_GCM_SHA384")                           \
  X(0xa2, DHE_DSS_AES128_GCM_SHA256, "TLS_DHE_DSS_WITH_AES_128_GCM_SHA256")                         \
  X(0xa3, DHE_DSS_AES256_GCM_SHA384, "TLS_DHE_DSS_WITH_AES_256_GCM_SHA384")                         \
  X(0xa4, DH_DSS_AES128_GCM_SHA256, "TLS_DH_DSS_WITH_AES_128_GCM_SHA256")                           \
  X(0xa5, DH_DSS_AES256_GCM_SHA384, "TLS_DH_DSS_WITH_AES_256_GCM_SHA384")                           \
  X(0xa6, ADH_AES128_GCM_SHA256, "TLS_DH_anon_WITH_AES_128_GCM_SHA256")                             \
  X(0xa7, ADH_AES256_GCM_SHA384, "TLS_DH_anon_WITH_AES_256_GCM_SHA384")                             \
  X(0xba, CAMELLIA128_SHA256, "TLS_RSA_WITH_CAMELLIA_128_CBC_SHA256")                               \
  X(0xbb, DH_DSS_CAMELLIA128_SHA256, "TLS_DH_DSS_WITH_CAMELLIA_128_CBC_SHA256")                     \
  X(0xbc, DH_RSA_CAMELLIA128_SHA256, "TLS_DH_RSA_WITH_CAMELLIA_128_CBC_SHA256")                     \
  X(0xbd, DHE_DSS_CAMELLIA128_SHA256, "TLS_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA256")                   \
  X(0xbe, DHE_RSA_CAMELLIA128_SHA256, "TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA256")                   \
  X(0xbf, ADH_CAMELLIA128_SHA256, "TLS_DH_anon_WITH_CAMELLIA_128_CBC_SHA256")                       \
  X(0x5600, TLS_FALLBACK_SCSV, "TLS_EMPTY_RENEGOTIATION_INFO_SCSV")                                 \
  X(0x1301, TLS_AES_128_GCM_SHA256, "TLS_AES_128_GCM_SHA256")                                       \
  X(0x1302, TLS_AES_256_GCM_SHA384, "TLS_AES_256_GCM_SHA384")                                       \
  X(0x1303, TLS_CHACHA20_POLY1305_SHA256, "TLS_CHACHA20_POLY1305_SHA256")                           \
  X(0x1304, TLS_AES_128_CCM_SHA256, "TLS_AES_128_CCM_SHA256")                                       \
  X(0x1305, TLS_AES_128_CCM_8_SHA256, "TLS_AES_128_CCM_8_SHA256")                                   \
  X(0xc001, ECDH_ECDSA_NULL_SHA, "TLS_ECDH_ECDSA_WITH_NULL_SHA")                                    \
  X(0xc002, ECDH_ECDSA_RC4_SHA, "TLS_ECDH_ECDSA_WITH_RC4_128_SHA")                                  \
  X(0xc003, ECDH_ECDSA_DES_CBC3_SHA, "TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA")                        \
  X(0xc004, ECDH_ECDSA_AES128_SHA, "TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA")                           \
  X(0xc005, ECDH_ECDSA_AES256_SHA, "TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA")                           \
  X(0xc006, ECDHE_ECDSA_NULL_SHA, "TLS_ECDHE_ECDSA_WITH_NULL_SHA")                                  \
  X(0xc007, ECDHE_ECDSA_RC4_SHA, "TLS_ECDHE_ECDSA_WITH_RC4_128_SHA")                                \
  X(0xc008, ECDHE_ECDSA_DES_CBC3_SHA, "TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA")                      \
  X(0xc009, ECDHE_ECDSA_AES128_SHA, "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA")                         \
  X(0xc00a, ECDHE_ECDSA_AES256_SHA, "TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA")                         \
  X(0xc00b, ECDH_RSA_NULL_SHA, "TLS_ECDH_RSA_WITH_NULL_SHA")                                        \
  X(0xc00c, ECDH_RSA_RC4_SHA, "TLS_ECDH_RSA_WITH_RC4_128_SHA")                                      \
  X(0xc00d, ECDH_RSA_DES_CBC3_SHA, "TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA")                            \
  X(0xc00e, ECDH_RSA_AES128_SHA, "TLS_ECDH_RSA_WITH_AES_128_CBC_SHA")                               \
  X(0xc00f, ECDH_RSA_AES256_SHA, "TLS_ECDH_RSA_WITH_AES_256_CBC_SHA")                               \
  X(0xc010, ECDHE_RSA_NULL_SHA, "TLS_ECDHE_RSA_WITH_NULL_SHA")                                      \
  X(0xc011, ECDHE_RSA_RC4_SHA, "TLS_ECDHE_RSA_WITH_RC4_128_SHA")                                    \
  X(0xc012, ECDHE_RSA_DES_CBC3_SHA, "TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA")                          \
  X(0xc013, ECDHE_RSA_AES128_SHA, "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA")                             \
  X(0xc014, ECDHE_RSA_AES256_SHA, "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA")                             \
  X(0xc015, AECDH_NULL_SHA, "TLS_ECDH_anon_WITH_NULL_SHA")                                          \
  X(0xc016, AECDH_RC4_SHA, "TLS_ECDH_anon_WITH_RC4_128_SHA")                                        \
  X(0xc017, AECDH_DES_CBC3_SHA, "TLS_ECDH_anon_WITH_3DES_EDE_CBC_SHA")                              \
  X(0xc018, AECDH_AES128_SHA, "TLS_ECDH_anon_WITH_AES_128_CBC_SHA")                                 \
  X(0xc019, AECDH_AES256_SHA, "TLS_ECDH_anon_WITH_AES_256_CBC_SHA")                                 \
  X(0xc01a, SRP_3DES_EDE_CBC_SHA, "TLS_SRP_SHA_WITH_3DES_EDE_CBC_SHA")                              \
  X(0xc01b, SRP_RSA_3DES_EDE_CBC_SHA, "TLS_SRP_SHA_RSA_WITH_3DES_EDE_CBC_SHA")                      \
  X(0xc01c, SRP_DSS_3DES_EDE_CBC_SHA, "TLS_SRP_SHA_DSS_WITH_3DES_EDE_CBC_SHA")                      \
  X(0xc01d, SRP_AES_128_CBC_SHA, "TLS_SRP_SHA_WITH_AES_128_CBC_SHA")                                \
  X(0xc01e, SRP_RSA_AES_128_CBC_SHA, "TLS_SRP_SHA_RSA_WITH_AES_128_CBC_SHA")                        \
  X(0xc01f, SRP_DSS_AES_128_CBC_SHA, "TLS_SRP_SHA_DSS_WITH_AES_128_CBC_SHA")                        \
  X(0xc020, SRP_AES_256_CBC_SHA, "TLS_SRP_SHA_WITH_AES_256_CBC_SHA")                                \
  X(0xc021, SRP_RSA_AES_256_CBC_SHA, "TLS_SRP_SHA_RSA_WITH_AES_256_CBC_SHA")                        \
  X(0xc022, SRP_DSS_AES_256_CBC_SHA, "TLS_SRP_SHA_DSS_WITH_AES_256_CBC_SHA")                        \
  X(0xc023, ECDHE_ECDSA_AES128_SHA256, "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256")                   \
  X(0xc024, ECDHE_ECDSA_AES256_SHA384, "TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384")                   \
  X(0xc025, ECDH_ECDSA_AES128_SHA256, "TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256")                     \
  X(0xc026, ECDH_ECDSA_AES256_SHA384, "TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384")                     \
  X(0xc027, ECDHE_RSA_AES128_SHA256, "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256")                       \
  X(0xc028, ECDHE_RSA_AES256_SHA384, "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384")                       \
  X(0xc029, ECDH_RSA_AES128_SHA256, "TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256")                         \
  X(0xc02a, ECDH_RSA_AES256_SHA384, "TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384")                         \
  X(0xc02b, ECDHE_ECDSA_AES128_GCM_SHA256, "TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256")               \
  X(0xc02c, ECDHE_ECDSA_AES256_GCM_SHA384, "TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384")               \
  X(0xc02d, ECDH_ECDSA_AES128_GCM_SHA256, "TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256")                 \
  X(0xc02e, ECDH_ECDSA_AES256_GCM_SHA384, "TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384")                 \
  X(0xc02f, ECDHE_RSA_AES128_GCM_SHA256, "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256")                   \
  X(0xc030, ECDHE_RSA_AES256_GCM_SHA384, "TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384")                   \
  X(0xc031, ECDH_RSA_AES128_GCM_SHA256, "TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256")                     \
  X(0xc032, ECDH_RSA_AES256_GCM_SHA384, "TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384")                     \
  X(0xc033, ECDHE_PSK_RC4_SHA, "TLS_ECDHE_PSK_WITH_RC4_128_SHA")                                    \
  X(0xc034, ECDHE_PSK_3DES_EDE_CBC_SHA, "TLS_ECDHE_PSK_WITH_3DES_EDE_CBC_SHA")                      \
  X(0xc035, ECDHE_PSK_AES128_CBC_SHA, "TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA")                         \
  X(0xc036, ECDHE_PSK_AES256_CBC_SHA, "TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA")                         \
  X(0xc037, ECDHE_PSK_AES128_CBC_SHA256, "TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA256")                   \
  X(0xc038, ECDHE_PSK_AES256_CBC_SHA384, "TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA384")                   \
  X(0xc039, ECDHE_PSK_NULL_SHA, "TLS_ECDHE_PSK_WITH_NULL_SHA")                                      \
  X(0xc03A, ECDHE_PSK_NULL_SHA256, "TLS_ECDHE_PSK_WITH_NULL_SHA256")                                \
  X(0xc03B, ECDHE_PSK_NULL_SHA384, "TLS_ECDHE_PSK_WITH_NULL_SHA384")                                \
  X(0xc072, ECDHE_ECDSA_CAMELLIA128_SHA256, "TLS_ECDHE_ECDSA_WITH_CAMELLIA_128_CBC_SHA256")         \
  X(0xc073, ECDHE_ECDSA_CAMELLIA256_SHA38, "TLS_ECDHE_ECDSA_WITH_CAMELLIA_256_CBC_SHA384")          \
  X(0xc074, ECDH_ECDSA_CAMELLIA128_SHA256, "TLS_ECDH_ECDSA_WITH_CAMELLIA_128_CBC_SHA256")           \
  X(0xc075, ECDH_ECDSA_CAMELLIA256_SHA384, "TLS_ECDH_ECDSA_WITH_CAMELLIA_256_CBC_SHA384")           \
  X(0xc076, ECDHE_RSA_CAMELLIA128_SHA256, "TLS_ECDHE_RSA_WITH_CAMELLIA_128_CBC_SHA256")             \
  X(0xc077, ECDHE_RSA_CAMELLIA256_SHA384, "TLS_ECDHE_RSA_WITH_CAMELLIA_256_CBC_SHA384")             \
  X(0xc078, ECDH_RSA_CAMELLIA128_SHA256, "TLS_ECDH_RSA_WITH_CAMELLIA_128_CBC_SHA256")               \
  X(0xc079, ECDH_RSA_CAMELLIA256_SHA384, "TLS_ECDH_RSA_WITH_CAMELLIA_256_CBC_SHA384")               \
  X(0xc094, PSK_CAMELLIA128_SHA256, "TLS_PSK_WITH_CAMELLIA_128_CBC_SHA256")                         \
  X(0xc095, PSK_CAMELLIA256_SHA384, "TLS_PSK_WITH_CAMELLIA_256_CBC_SHA384")                         \
  X(0xc096, DHE_PSK_CAMELLIA128_SHA256, "TLS_DHE_PSK_WITH_CAMELLIA_128_CBC_SHA256")                 \
  X(0xc097, DHE_PSK_CAMELLIA256_SHA384, "TLS_DHE_PSK_WITH_CAMELLIA_256_CBC_SHA384")                 \
  X(0xc098, RSA_PSK_CAMELLIA128_SHA256, "TLS_RSA_PSK_WITH_CAMELLIA_128_CBC_SHA256")                 \
  X(0xc099, RSA_PSK_CAMELLIA256_SHA384, "TLS_RSA_PSK_WITH_CAMELLIA_256_CBC_SHA384")                 \
  X(0xc09A, ECDHE_PSK_CAMELLIA128_SHA256, "TLS_ECDHE_PSK_WITH_CAMELLIA_128_CBC_SHA256")             \
  X(0xc09B, ECDHE_PSK_CAMELLIA256_SHA384, "TLS_ECDHE_PSK_WITH_CAMELLIA_256_CBC_SHA384")             \
  X(0xc09c, AES128_CCM, "TLS_RSA_WITH_AES_128_CCM")                                                 \
  X(0xc09d, AES256_CCM, "TLS_RSA_WITH_AES_256_CCM")                                                 \
  X(0xc09e, DHE_RSA_AES128_CCM, "TLS_DHE_RSA_WITH_AES_128_CCM")                                     \
  X(0xc09f, DHE_RSA_AES256_CCM, "TLS_DHE_RSA_WITH_AES_256_CCM")                                     \
  X(0xc0a0, AES128_CCM8, "TLS_RSA_WITH_AES_128_CCM_8")                                              \
  X(0xc0a1, AES256_CCM8, "TLS_RSA_WITH_AES_256_CCM_8")                                              \
  X(0xc0a2, DHE_RSA_AES128_CCM8, "TLS_DHE_RSA_WITH_AES_128_CCM_8")                                  \
  X(0xc0a3, DHE_RSA_AES256_CCM8, "TLS_DHE_RSA_WITH_AES_256_CCM_8")                                  \
  X(0xc0a4, PSK_AES128_CCM, "TLS_PSK_WITH_AES_128_CCM")                                             \
  X(0xc0a5, PSK_AES256_CCM, "TLS_PSK_WITH_AES_256_CCM")                                             \
  X(0xc0a6, DHE_PSK_AES128_CCM, "TLS_DHE_PSK_WITH_AES_128_CCM")                                     \
  X(0xc0a7, DHE_PSK_AES256_CCM, "TLS_DHE_PSK_WITH_AES_256_CCM")                                     \
  X(0xc0a8, PSK_AES128_CCM8, "TLS_PSK_WITH_AES_128_CCM_8")                                          \
  X(0xc0a9, PSK_AES256_CCM8, "TLS_PSK_WITH_AES_256_CCM_8")                                          \
  X(0xc0aa, DHE_PSK_AES128_CCM8, "TLS_PSK_DHE_WITH_AES_128_CCM_8")                                  \
  X(0xc0ab, DHE_PSK_AES256_CCM8, "TLS_PSK_DHE_WITH_AES_256_CCM_8")                                  \
  X(0xc0ac, ECDHE_ECDSA_AES128_CCM, "TLS_ECDHE_ECDSA_WITH_AES_128_CCM")                             \
  X(0xc0ad, ECDHE_ECDSA_AES256_CCM, "TLS_ECDHE_ECDSA_WITH_AES_256_CCM")                             \
  X(0xc0ae, ECDHE_ECDSA_AES128_CCM8, "TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8")                          \
  X(0xc0af, ECDHE_ECDSA_AES256_CCM8, "TLS_ECDHE_ECDSA_WITH_AES_256_CCM_8")                          \
  X(0xcc13, ECDHE_RSA_CHACHA20_POLY1305_OLD, "TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256_OLD")     \
  X(0xcc14, ECDHE_ECDSA_CHACHA20_POLY1305_OLD, "TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256_OLD") \
  X(0xcc15, DHE_RSA_CHACHA20_POLY1305_OLD, "TLS_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256_OLD")         \
  X(0xcca8, ECDHE_RSA_CHACHA20_POLY1305, "TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256")             \
  X(0xcca9, ECDHE_ECDSA_CHACHA20_POLY1305, "TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256")         \
  X(0xccaa, DHE_RSA_CHACHA20_POLY1305, "TLS_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256")                 \
  X(0xccab, PSK_CHACHA20_POLY1305, "TLS_PSK_WITH_CHACHA20_POLY1305_SHA256")                         \
  X(0xccac, ECDHE_PSK_CHACHA20_POLY1305, "TLS_ECDHE_PSK_WITH_CHACHA20_POLY1305_SHA256")             \
  X(0xccad, DHE_PSK_CHACHA20_POLY1305, "TLS_DHE_PSK_WITH_CHACHA20_POLY1305_SHA256")                 \
  X(0xccae, RSA_PSK_CHACHA20_POLY1305, "TLS_RSA_PSK_WITH_CHACHA20_POLY1305_SHA256")                 \
  X(0xff00, GOST_MD5, "TLS_GOSTR341094_RSA_WITH_28147_CNT_MD5")                                     \
  X(0xff01, GOST_GOST94, "TLS_RSA_WITH_28147_CNT_GOST94")                                           \
  X(0x010080, RC4_MD5__1, "SSL_CK_RC4_128_WITH_MD5")                                                \
  X(0x020080, EXP_RC4_MD5__1, "SSL_CK_RC4_128_EXPORT40_WITH_MD5")                                   \
  X(0x030080, RC2_CBC_MD5, "SSL_CK_RC2_128_CBC_WITH_MD5")                                           \
  X(0x040080, EXP_RC2_CBC_MD5__1, "SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5")                           \
  X(0x050080, IDEA_CBC_MD5, "SSL_CK_IDEA_128_CBC_WITH_MD5")                                         \
  X(0x060040, DES_CBC_MD5, "SSL_CK_DES_64_CBC_WITH_MD5")                                            \
  X(0x060140, DES_CBC_SHA__1, "SSL_CK_DES_64_CBC_WITH_SHA")                                         \
  X(0x0700c0, DES_CBC3_MD5, "SSL_CK_DES_192_EDE3_CBC_WITH_MD5")                                     \
  X(0x0701c0, DES_CBC3_SHA__1, "SSL_CK_DES_192_EDE3_CBC_WITH_SHA")                                  \
  X(0x080080, RC4_64_MD5, "SSL_CK_RC4_64_WITH_MD5")                                                 \
  X(0xff0800, DES_CFB_M1, "SSL_CK_DES_64_CFB64_WITH_MD5_1")                                         \
  X(0xff0810, _NULL, "SSL_CK_NULL")

enum class cipher_suite_name {
#define X(id, name, str) name = id,
  CIPHER_SUITE_NAMES(X)
#undef X
};

constexpr std::string_view cipher_to_string(cipher_suite_name cipher) {
  switch (cipher) {
#define X(id, name, str)        \
  case cipher_suite_name::name: \
    return str;
    CIPHER_SUITE_NAMES(X)
#undef X
  }
  throw error::tls::invalid_cipher_suite_exception{};
}

bool cipher_is_valid(cipher_suite_name cipher);

std::ostream& operator<<(std::ostream& output, cipher_suite_name cipher);

}  // namespace proxy::tls::handshake
