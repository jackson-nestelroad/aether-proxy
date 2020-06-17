/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "handshake_reader.hpp"

namespace proxy::tcp::tls::handshake {
    handshake_reader::handshake_reader()
        : length(0),
        known_length(false)
    { }

    std::size_t handshake_reader::read(std::istream &in) {
        // Length is not known, meaning we have not read the record header
        if (!known_length) {
            // Read record header
            bool read_header = header_segment.read_up_to_bytes(in, record_header_length);
            // Didn't even have enough data to read the record header
            if (!read_header) {
                return record_header_length - header_segment.bytes_read();
            }
            // Got the record header
            else {
                auto &bytes = header_segment.data_ref();
                // Works for SSLv3, TLSv1.0, TLSv1.1, TLSv1.2
                bool is_tls_record =
                    bytes[0] == 0x16
                    && bytes[1] == 0x03
                    && bytes[2] >= 0x00
                    && bytes[2] <= 0x03;

                // We should NOT read this message if it is not TLS
                if (!is_tls_record) {
                    throw error::tls::invalid_client_hello_exception { };
                }
                length = (bytes[3] << 8) | bytes[4];
                known_length = true;
            }
        }
        // Length is known, so try to get everything remaining
        // No need to use a buffer_segment since we can always own this data
        record_segment.read_up_to_bytes(in, length);
        return length - record_segment.bytes_read();
    }

    byte_array handshake_reader::get_bytes() const {
        byte_array bytes;
        header_segment.copy_data(std::back_inserter(bytes));
        record_segment.copy_data(std::back_inserter(bytes));
        return bytes;
    }

    void handshake_reader::reset() {
        header_segment.reset();
        record_segment.reset();
        length = 0;
        known_length = false;
    }
}