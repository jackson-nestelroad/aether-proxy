/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "client_hello.hpp"

namespace proxy::tcp::tls::handshake {
    client_hello client_hello::from_raw_data(const byte_array &raw_data) {
        try {
            const std::size_t data_length = raw_data.size();
            std::size_t prev_index = 0;
            std::size_t index = 0;

            // Bytes needed for record header and handshake header
            if (data_length < 9) {
                throw error::tls::invalid_client_hello_exception { };
            }

            client_hello result;
            copy_bytes(raw_data, result.record_header, index, 5);
            copy_bytes(raw_data, result.handshake_header, index, 4);


            // Must be a Client Hello message
            if (result.handshake_header[0] != 0x01) {
                throw error::tls::invalid_client_hello_exception { };
            }

            // This check is somewhat arbitrary, since we do quite a few bounds checks along the way
            std::size_t handshake_length = util::bytes::concat(
                result.handshake_header[1], result.handshake_header[2], result.handshake_header[3]);
            if (handshake_length != data_length - 9) {
                throw error::tls::invalid_client_hello_exception { };
            }

            // Read version and random
            copy_bytes(raw_data, result.version, index, 2);
            copy_bytes(raw_data, result.random, index, 32);

            // Read session id
            std::size_t session_id_length = raw_data[index++];
            copy_bytes(raw_data, result.session_id, index, session_id_length);

            // Read cipher suites
            std::size_t ciphers_length = read_byte_string(raw_data, index, 2);
            prev_index = index;
            while (index < prev_index + ciphers_length) {
                result.cipher_suites.push_back(static_cast<cipher_suite_name>(read_byte_string(raw_data, index, 2)));
            }
            // Bounds check for cipher suites
            if (index != prev_index + ciphers_length) {
                throw error::tls::invalid_client_hello_exception { "Invalid cipher suites length" };
            }

            // Read compression methods
            std::size_t compression_length = raw_data[index++];
            copy_bytes(raw_data, result.compression_methods, index, compression_length);

            // Read extensions, if they exist
            std::size_t extensions_length = read_byte_string(raw_data, index, 2);
            if (extensions_length != 0) {
                // Extensions read to the end of the message, so we just check current index to the total data length
                while (index < data_length) {
                    std::size_t type = read_byte_string(raw_data, index, 2);
                    std::size_t length = read_byte_string(raw_data, index, 2);

                    // We are specifically interested in a few extensions, so we do a bit more parsing on them

                    if (type == static_cast<std::size_t>(extension_type::server_name)) {
                        prev_index = index;
                        while (index < prev_index + length) {
                            std::size_t entry_length = read_byte_string(raw_data, index, 2);
                            server_name entry;
                            entry.type = raw_data[index++];
                            std::size_t name_length = read_byte_string(raw_data, index, 2);
                            copy_bytes(raw_data, entry.host_name, index, name_length);
                            result.server_names.push_back(entry);
                        }
                        // Bounds check for server name extension
                        if (index != prev_index + length) {
                            throw error::tls::invalid_client_hello_exception { "Error in parsing server name extension" };
                        }
                    }
                    else if (type == static_cast<std::size_t>(extension_type::application_layer_protocol_negotiation)) {
                        std::size_t alpn_length = read_byte_string(raw_data, index, 2);
                        prev_index = index;
                        while (index < prev_index + alpn_length) {
                            std::size_t entry_length = raw_data[index++];
                            auto &next_alpn = result.alpn.emplace_back();
                            copy_bytes(raw_data, next_alpn, index, entry_length);
                        }
                        // Bounds check for alpn extension
                        if (index != prev_index + alpn_length) {
                            throw error::tls::invalid_client_hello_exception { "Error in parsing ALPN extension" };
                        }
                    }
                    else {
                        auto pair = result.extensions.emplace(static_cast<extension_type>(type), byte_array { });
                        if (!pair.second) {
                            throw error::tls::invalid_client_hello_exception { "Duplicate extension found" };
                        }
                        copy_bytes(raw_data, pair.first->second, index, length);
                    }
                }
            }

            // Bounds check for entire message
            if (index != data_length) {
                throw error::tls::invalid_client_hello_exception { "Invalid Client Hello length" };
            }
            return result;
        }
        // This error is thrown anytime the data array is read past its end
        catch (const error::tls::read_access_violation_exception &) {
            throw error::tls::invalid_client_hello_exception { "Not enough data available" };
        }
    }

    std::size_t client_hello::read_byte_string(const byte_array &src, std::size_t &offset, std::size_t num_bytes) {
        if (offset + num_bytes > src.size()) {
            throw error::tls::read_access_violation_exception { };
        }
        std::size_t res = 0;
        for (std::size_t i = 0; i < num_bytes; ++i) {
            res <<= 8;
            res |= src[offset++];
        }
        return res;
    }

    bool client_hello::has_server_names_extension() const {
        return server_names.size() > 0;
    }

    bool client_hello::has_alpn_extension() const {
        return alpn.size() > 0;
    }
}