/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/types.hpp>
#include <aether/proxy/tcp/tls/handshake/handshake_types.hpp>
#include <aether/proxy/connection/base_connection.hpp>
#include <aether/proxy/error/exceptions.hpp>
#include <aether/util/bytes.hpp>

namespace proxy::tcp::tls::handshake {
    /*
        Data structure for accessing and manipulating various parts
            of the TLS Client Hello message.
    */
    struct client_hello {
    private:
        template <typename Container>
        static void copy_bytes(const const_buffer &src, Container &dest, std::size_t &offset, std::size_t num_bytes) {
            if (offset + num_bytes > src.size()) {
                throw error::tls::read_access_violation_exception { };
            }
            std::copy(src.begin() + offset, src.begin() + offset + num_bytes, std::back_inserter(dest));
            offset += num_bytes;
        }

        static std::size_t read_byte_string(const const_buffer &src, std::size_t &offset, std::size_t num_bytes);

    public:
        struct server_name {
            byte_t type;
            std::string host_name;
        };

        // Parts of the handshake message

        byte_array record_header;
        byte_array handshake_header;
        byte_array version;
        byte_array random;
        byte_array session_id;
        std::vector<cipher_suite_name> cipher_suites;
        byte_array compression_methods;
        std::map<extension_type, byte_array> extensions;

        // Parsed extensions (not in extensions map above)

        std::vector<server_name> server_names;
        std::vector<std::string> alpn;

        bool has_server_names_extension() const;
        bool has_alpn_extension() const;

        /*
            Parses a Client Hello message into its corresponding data structure.
        */
        static client_hello from_raw_data(const const_buffer &raw_data);
    };
}
