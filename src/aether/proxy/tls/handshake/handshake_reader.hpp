/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include "aether/proxy/types.hpp"
#include "aether/util/buffer_segment.hpp"

namespace proxy::tls::handshake {

// Reads the raw handshake data for it to be parsed later.
class handshake_reader {
 public:
  static constexpr std::size_t record_header_length = 5;

  handshake_reader();
  ~handshake_reader() = default;
  handshake_reader(const handshake_reader& other) = delete;
  handshake_reader& operator=(const handshake_reader& other) = delete;
  handshake_reader(handshake_reader&& other) noexcept = delete;
  handshake_reader& operator=(handshake_reader&& other) = delete;

  // Reads the raw Client Hello message from the input stream.
  // Returns the number of bytes needed to complete the record.
  // Returns 0 if finished.
  std::size_t read(const_buffer& buf, std::size_t bytes_available);

  const_buffer get_bytes() const;
  void insert_into_stream(std::ostream& out);
  void reset();

 private:
  util::buffer::const_buffer_segment segment_;
  std::uint16_t length_;
  bool known_length_;
};

}  // namespace proxy::tls::handshake
