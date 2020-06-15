/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/types.hpp>
#include <aether/proxy/connection/base_connection.hpp>
#include <aether/proxy/tcp/util/buffer.hpp>

namespace proxy::tcp::tls::handshake {
    using byte_array = std::vector<std::uint8_t>;

    class client_hello {
    private:
        byte_array record_header;
        byte_array handshake_header;
        byte_array version;
        byte_array random;
        byte_array session_id;
        byte_array cipher_suites;
        byte_array compression_methods;
        std::map<std::size_t, byte_array> extensions;

        /*
            Parses a ClientHello message into its corresponding data structure.
        */
        static client_hello from_raw_data(const byte_array &raw_data);
    };
}
