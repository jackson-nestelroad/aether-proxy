/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "client_hello.hpp"

#include "aether/proxy/error/error.hpp"
#include "aether/proxy/tls/handshake/handshake_types.hpp"
#include "aether/proxy/types.hpp"
#include "aether/util/bytes.hpp"
#include "aether/util/result_macros.hpp"
#include "aether/util/streambuf.hpp"

namespace proxy::tls::handshake {

result<client_hello> client_hello::from_raw_data_impl(const const_buffer& raw_data) {
  const std::size_t data_length = raw_data.size();
  std::size_t prev_index = 0;
  std::size_t index = 0;

  // Bytes needed for record header and handshake header.
  if (data_length < 9) {
    return error::tls::invalid_client_hello();
  }

  client_hello result_message;
  RETURN_IF_ERROR(copy_bytes(raw_data, result_message.record_header, index, 5));
  RETURN_IF_ERROR(copy_bytes(raw_data, result_message.handshake_header, index, 4));

  // Must be a Client Hello message.
  if (result_message.handshake_header[0] != 0x01) {
    return error::tls::invalid_client_hello();
  }

  // This check is somewhat arbitrary, since we do quite a few bounds checks along the way.
  std::size_t handshake_length = util::bytes::concat(
      result_message.handshake_header[1], result_message.handshake_header[2], result_message.handshake_header[3]);
  if (handshake_length != data_length - 9) {
    return error::tls::invalid_client_hello();
  }

  // Read version and random.
  RETURN_IF_ERROR(copy_bytes(raw_data, result_message.version, index, 2));
  RETURN_IF_ERROR(copy_bytes(raw_data, result_message.random, index, 32));

  // Read session id.
  std::size_t session_id_length = raw_data[index++];
  RETURN_IF_ERROR(copy_bytes(raw_data, result_message.session_id, index, session_id_length));

  // Read cipher suites.
  ASSIGN_OR_RETURN(size_t ciphers_length, read_byte_string(raw_data, index, 2));
  prev_index = index;
  while (index < prev_index + ciphers_length) {
    ASSIGN_OR_RETURN(size_t cipher_suite, read_byte_string(raw_data, index, 2));
    result_message.cipher_suites.push_back(static_cast<cipher_suite_name>(cipher_suite));
  }
  // Bounds check for cipher suites.
  if (index != prev_index + ciphers_length) {
    return error::tls::invalid_client_hello("Invalid cipher suites length");
  }

  // Read compression methods.
  std::size_t compression_length = raw_data[index++];
  RETURN_IF_ERROR(copy_bytes(raw_data, result_message.compression_methods, index, compression_length));

  // Read extensions, if they exist.
  ASSIGN_OR_RETURN(size_t extensions_length, read_byte_string(raw_data, index, 2));
  if (extensions_length != 0) {
    // Extensions read to the end of the message, so we just check current index to the total data length.
    while (index < data_length) {
      ASSIGN_OR_RETURN(size_t type, read_byte_string(raw_data, index, 2));
      ASSIGN_OR_RETURN(std::size_t length, read_byte_string(raw_data, index, 2));

      // We are specifically interested in a few extensions, so we do a bit more parsing on them.

      if (type == static_cast<std::size_t>(extension_type::server_name)) {
        prev_index = index;
        while (index < prev_index + length) {
          RETURN_IF_ERROR(read_byte_string(raw_data, index, 2));
          server_name entry;
          entry.type = raw_data[index++];
          ASSIGN_OR_RETURN(std::size_t name_length, read_byte_string(raw_data, index, 2));
          RETURN_IF_ERROR(copy_bytes(raw_data, entry.host_name, index, name_length));
          result_message.server_names.push_back(entry);
        }
        // Bounds check for server name extension.
        if (index != prev_index + length) {
          return error::tls::invalid_client_hello("Error in parsing server name extension");
        }
      } else if (type == static_cast<std::size_t>(extension_type::application_layer_protocol_negotiation)) {
        ASSIGN_OR_RETURN(size_t alpn_length, read_byte_string(raw_data, index, 2));
        prev_index = index;
        while (index < prev_index + alpn_length) {
          std::size_t entry_length = raw_data[index++];
          auto& next_alpn = result_message.alpn.emplace_back();
          RETURN_IF_ERROR(copy_bytes(raw_data, next_alpn, index, entry_length));
        }
        // Bounds check for alpn extension.
        if (index != prev_index + alpn_length) {
          return error::tls::invalid_client_hello("Error in parsing ALPN extension");
        }
      } else {
        auto pair = result_message.extensions.emplace(static_cast<extension_type>(type), byte_array_t{});
        if (!pair.second) {
          return error::tls::invalid_client_hello("Duplicate extension found");
        }
        RETURN_IF_ERROR(copy_bytes(raw_data, pair.first->second, index, length));
      }
    }
  }

  // Bounds check for entire message.
  if (index != data_length) {
    return error::tls::invalid_client_hello("Invalid Client Hello length");
  }
  return result_message;
}

result<client_hello> client_hello::from_raw_data(const const_buffer& raw_data) {
  result<client_hello> message = from_raw_data_impl(raw_data);
  if (!message.is_ok()) {
    if (message.err().proxy_error() == errc::read_access_violation) {
      return error::tls::invalid_client_hello("Invalid Client Hello length");
    }
  }
  return message;
}

result<std::size_t> client_hello::read_byte_string(const const_buffer& src, std::size_t& offset,
                                                   std::size_t num_bytes) {
  if (offset + num_bytes > src.size()) {
    return error::tls::read_access_violation();
  }
  std::size_t res = 0;
  for (std::size_t i = 0; i < num_bytes; ++i) {
    res <<= 8;
    res |= static_cast<byte_t>(src[offset++]);
  }
  return res;
}

bool client_hello::has_server_names_extension() const { return server_names.size() > 0; }

bool client_hello::has_alpn_extension() const { return alpn.size() > 0; }

}  // namespace proxy::tls::handshake
