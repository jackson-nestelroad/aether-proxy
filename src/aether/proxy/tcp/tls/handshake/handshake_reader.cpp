/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "handshake_reader.hpp"

#include "aether/proxy/error/exceptions.hpp"
#include "aether/util/bytes.hpp"

namespace proxy::tcp::tls::handshake {

handshake_reader::handshake_reader() : length_(0), known_length_(false) {}

std::size_t handshake_reader::read(const_buffer& buf, std::size_t bytes_available) {
  if (!known_length_) {
    // Read record header.
    bool read_header = segment_.read_up_to_bytes(buf, record_header_length, bytes_available);
    if (!read_header) {
      // Didn't even have enough data to read the record header.
      return record_header_length - segment_.bytes_not_committed();
    }

    // Got the record header.
    segment_.mark_as_incomplete();
    auto bytes = segment_.committed_data().data();
    // Works for SSLv3, TLSv1.0, TLSv1.1, TLSv1.2.
    bool is_tls_record = bytes[0] == 0x16 && bytes[1] == 0x03 && bytes[2] >= 0x00 && bytes[2] <= 0x03;

    // We should NOT read this message if it is not TLS.
    if (!is_tls_record) {
      throw error::tls::invalid_client_hello_exception{};
    }
    length_ = static_cast<std::uint16_t>(util::bytes::concat(bytes[3], bytes[4]));
    known_length_ = true;
  }

  // Length is known, so try to get everything remaining.
  if (!segment_.read_up_to_bytes(buf, length_, bytes_available)) {
    return length_ - segment_.bytes_not_committed();
  }
  return 0;
}

const_buffer handshake_reader::get_bytes() const { return segment_.committed_data(); }

void handshake_reader::insert_into_stream(std::ostream& out) { out << segment_.committed_data().data(); }

void handshake_reader::reset() {
  segment_.reset();
  length_ = 0;
  known_length_ = false;
}

}  // namespace proxy::tcp::tls::handshake
