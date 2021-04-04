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

    std::size_t handshake_reader::read(const_buffer &buf, std::size_t bytes_available) {
        if (!known_length) {
            // Read record header
            bool read_header = segment.read_up_to_bytes(buf, record_header_length, bytes_available);
            // Didn't even have enough data to read the record header
            if (!read_header) {
                return record_header_length - segment.bytes_not_committed();
            }
            // Got the record header
            else {
                segment.mark_as_incomplete();
                auto bytes = segment.committed_data().data();
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
                length = static_cast<std::uint16_t>(util::bytes::concat(bytes[3], bytes[4]));
                known_length = true;
            }
        }
        // Length is known, so try to get everything remaining
        if (!segment.read_up_to_bytes(buf, length, bytes_available)) {
            return length - segment.bytes_not_committed();
        }
        return 0;
    }

    const_buffer handshake_reader::get_bytes() const {
        return segment.committed_data();
    }
    
    void handshake_reader::insert_into_stream(std::ostream &out) {
        out << segment.committed_data().data();
    }

    void handshake_reader::reset() {
        segment.reset();
        length = 0;
        known_length = false;
    }
}
