/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "parser.hpp"

namespace proxy::tcp::tls::handshake {
    parser::parser()
        : bytes(),
        length(0),
        remaining(0),
        known_length(false)
    { }

    std::size_t parser::read(std::istream &in) {
        // Length is not known, meaning we have not read the record header
        if (!known_length) {
            // Read record header
            bytes.resize(5);
            std::size_t just_read = peek(in, 5);
            // Didn't even have enough data to read the record header
            if (just_read != 5) {
                // Discard bytes, we'll just read them again
                bytes.clear();
                return 5;
            }
            // Got the record header
            else {
                // Works for SSLv3, TLSv1.0, TLSv1.1, TLSv1.2
                bool is_tls_record =
                    bytes[0] == 0x16
                    && bytes[1] == 0x03
                    && bytes[2] >= 0x00
                    && bytes[2] <= 0x03;

                // We should NOT read this message if it is not TLS
                if (!is_tls_record) {
                    bytes.clear();
                    throw error::tls::invalid_client_hello_exception { };
                }
                remaining = length = (bytes[3] << 8) | bytes[4];
            }
        }
        // Length is known, so try to get everything remaining
        std::size_t just_read = peek(in, remaining);
        remaining -= just_read;
        return remaining;
    }

    std::size_t parser::peek(std::istream &in, std::size_t num_bytes) {
        in.read(reinterpret_cast<char *>(bytes.data()), num_bytes);
        auto just_read = in.gcount();
        unget(in, just_read);
        return just_read;
    }

    void parser::unget(std::istream &in, std::size_t num_bytes) {
        for (std::size_t i = 0; i < num_bytes; ++i) {
            in.unget();
        }
    }

    byte_array parser::get_bytes() const {
        return bytes;
    }

    std::size_t parser::get_remaining() const {
        return remaining;
    }

    bool parser::is_finished() const {
        return known_length && remaining == 0;
    }
}