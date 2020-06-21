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
    class client_hello {
    public:
        struct server_name {
            byte type;
            byte_array host_name;
        };

    private:
        byte_array record_header;
        byte_array handshake_header;
        byte_array version;
        byte_array random;
        byte_array session_id;
        std::vector<cipher_suite_name> cipher_suites;
        byte_array compression_methods;
        std::map<extension_type, byte_array> extensions;

        std::vector<server_name> sni;
        std::vector<byte_array> alpn;

        static void copy_bytes(const byte_array &src, byte_array &dest, std::size_t &offset, std::size_t num_bytes);
        static std::size_t read_byte_string(const byte_array &src, std::size_t &offset, std::size_t num_bytes);

    public:
        /*
            Parses a ClientHello message into its corresponding data structure.
        */
        static client_hello from_raw_data(const byte_array &raw_data);
    };
}
