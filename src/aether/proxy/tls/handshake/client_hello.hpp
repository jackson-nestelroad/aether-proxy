/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <algorithm>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#include "aether/proxy/connection/base_connection.hpp"
#include "aether/proxy/error/error.hpp"
#include "aether/proxy/tls/handshake/handshake_types.hpp"
#include "aether/proxy/types.hpp"
#include "aether/util/streambuf.hpp"

namespace proxy::tls::handshake {

// Data structure for accessing and manipulating various parts of the TLS Client Hello message.
struct client_hello {
 public:
  struct server_name {
    byte_t type;
    std::string host_name;
  };

  // Parses a Client Hello message into its corresponding data structure.
  static result<client_hello> from_raw_data(const const_buffer& raw_data);

  client_hello() = default;
  ~client_hello() = default;
  client_hello(const client_hello& other) = default;
  client_hello& operator=(const client_hello& other) = default;
  client_hello(client_hello&& other) noexcept = default;
  client_hello& operator=(client_hello&& other) noexcept = default;

  // Parts of the handshake message.

  byte_array_t record_header;
  byte_array_t handshake_header;
  byte_array_t version;
  byte_array_t random;
  byte_array_t session_id;
  std::vector<cipher_suite_name> cipher_suites;
  byte_array_t compression_methods;
  std::map<extension_type, byte_array_t> extensions;

  // Parsed extensions (not in extensions map above).

  std::vector<server_name> server_names;
  std::vector<std::string> alpn;

  bool has_server_names_extension() const;
  bool has_alpn_extension() const;

 private:
  static result<client_hello> from_raw_data_impl(const const_buffer& raw_data);

  template <typename Container>
  static result<void> copy_bytes(const const_buffer& src, Container& dest, std::size_t& offset, std::size_t num_bytes) {
    if (offset + num_bytes > src.size()) {
      return error::tls::read_access_violation();
    }
    std::copy(src.begin() + offset, src.begin() + offset + num_bytes, std::back_inserter(dest));
    offset += num_bytes;
    return util::ok;
  }

  static result<std::size_t> read_byte_string(const const_buffer& src, std::size_t& offset, std::size_t num_bytes);
};
}  // namespace proxy::tls::handshake
