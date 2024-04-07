/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "frame_parser.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <utility>
#include <vector>

#include "aether/proxy/error/error.hpp"
#include "aether/proxy/websocket/message/endpoint.hpp"
#include "aether/proxy/websocket/message/frame.hpp"
#include "aether/proxy/websocket/protocol/extensions/extension.hpp"
#include "aether/util/buffer_segment.hpp"
#include "aether/util/bytes.hpp"
#include "aether/util/console.hpp"
#include "aether/util/double_buffer.hpp"
#include "aether/util/streambuf.hpp"

namespace proxy::websocket::protocol {

frame_parser::frame_parser(endpoint destination, const std::vector<handshake::extension_data>& extension_data)
    : destination_(destination),
      state_(parsing_state::header),
      current_frame_(),
      effective_opcode_in_(),
      effective_opcode_out_() {
  for (const auto& data : extension_data) {
    result<std::unique_ptr<extensions::extension>> extension_instance =
        extensions::extension::from_extension_data(destination, data);
    if (extension_instance.is_ok() && extension_instance.ok() != nullptr) {
      extensions_.emplace_back(std::move(extension_instance).ok());
    }
  }
}

void frame_parser::xor_mask(std::uint32_t key, std::streambuf& input, std::streambuf& output) {
  std::array<byte_t, 4> xor_mask_key = {util::bytes::extract_byte<4>(key), util::bytes::extract_byte<3>(key),
                                        util::bytes::extract_byte<2>(key), util::bytes::extract_byte<1>(key)};
  std::size_t i = 0;

  std::transform(std::istreambuf_iterator<char>(&input), std::istreambuf_iterator<char>(),
                 std::ostreambuf_iterator<char>(&output),
                 [&i, &xor_mask_key](byte_t b) { return b ^ xor_mask_key[(i++) & 3]; });
}

result<std::optional<frame>> frame_parser::parse(streambuf& in, std::optional<close_code>& should_close) {
  if (state_ == parsing_state::header) {
    // Parse first two bytes.
    if (!header_segment_.read_up_to_bytes(in, 2)) {
      return std::nullopt;
    }

    std::string_view data = header_segment_.string_view();
    byte_t first_byte = data[0];
    byte_t second_byte = data[1];
    header_segment_.reset();

    current_frame_.fin = first_byte & static_cast<byte_t>(first_byte_mask::fin);
    current_frame_.rsv.rsv1 = first_byte & static_cast<byte_t>(first_byte_mask::rsv1);
    current_frame_.rsv.rsv2 = first_byte & static_cast<byte_t>(first_byte_mask::rsv2);
    current_frame_.rsv.rsv3 = first_byte & static_cast<byte_t>(first_byte_mask::rsv3);

    byte_t raw_opcode = first_byte & static_cast<byte_t>(first_byte_mask::opcode);
    if (raw_opcode > static_cast<byte_t>(opcode::max)) {
      return error::websocket::invalid_frame("Invalid opcode");
    }
    current_frame_.type = static_cast<opcode>(raw_opcode);
    if (is_control(current_frame_.type) && !current_frame_.fin) {
      return error::websocket::invalid_frame("Cannot fragment control frames");
    } else if (effective_opcode_in_.has_value() && current_frame_.type != opcode::continuation) {
      return error::websocket::invalid_frame("Expected a fragmented continuation frame");
    }

    current_frame_.mask_bit = second_byte & static_cast<byte_t>(second_byte_mask::mask);

    current_frame_.payload_length = second_byte & static_cast<byte_t>(second_byte_mask::payload_length);
    if (is_control(current_frame_.type) &&
        current_frame_.payload_length > static_cast<std::size_t>(payload_constants::max_one_byte)) {
      return error::websocket::invalid_frame("Control frame payload cannot exceed 125");
    }

    state_ = parsing_state::payload_length;
  }

  if (state_ == parsing_state::payload_length) {
    // Parse extended payload length.
    if (current_frame_.payload_length == static_cast<std::size_t>(payload_constants::payload_length_two_byte)) {
      if (!payload_length_segment_.read_up_to_bytes(in, 2)) {
        return std::nullopt;
      }

      current_frame_.payload_length =
          util::bytes::parse_network_byte_order<2>(payload_length_segment_.committed_data());
      payload_length_segment_.reset();
      if (current_frame_.payload_length <= static_cast<std::size_t>(payload_constants::max_one_byte)) {
        return error::websocket::invalid_frame("Payload length did not encode with minimum bytes");
      }
    } else if (current_frame_.payload_length ==
               static_cast<std::size_t>(payload_constants::payload_length_eight_byte)) {
      if (!payload_length_segment_.read_up_to_bytes(in, 8)) {
        return std::nullopt;
      }

      current_frame_.payload_length =
          util::bytes::parse_network_byte_order<8>(payload_length_segment_.committed_data());
      payload_length_segment_.reset();
      if (current_frame_.payload_length <= static_cast<std::size_t>(payload_constants::max_two_byte)) {
        return error::websocket::invalid_frame("Payload length did not encode with minimum bytes");
      }

      if (current_frame_.payload_length >> 63) {
        return error::websocket::invalid_frame("MSB must be 0 when using eight-byte payload length");
      }
    }

    // Run extensions.
    for (const std::unique_ptr<extensions::extension>& ext : extensions_) {
      extensions::extension::hook_return result = ext->on_inbound_frame_header(current_frame_);
      if (result.close.has_value()) {
        should_close.emplace(result.close.value());
        return std::nullopt;
      }
    }

    if (current_frame_.mask_bit && destination_ == endpoint::client) {
      return error::websocket::invalid_frame("Client received unexpected masked frame");
    }
    if (!current_frame_.mask_bit && destination_ == endpoint::server) {
      return error::websocket::invalid_frame("Server received unexpected unmasked frame");
    }

    state_ = parsing_state::mask_key;
  }

  if (state_ == parsing_state::mask_key) {
    if (current_frame_.mask_bit) {
      if (!mask_key_segment_.read_up_to_bytes(in, 4)) {
        return std::nullopt;
      }

      current_frame_.mask_key =
          static_cast<std::uint32_t>(util::bytes::parse_network_byte_order<8>(mask_key_segment_.committed_data()));
      mask_key_segment_.reset();
    }

    state_ = parsing_state::payload;
  }

  if (state_ == parsing_state::payload) {
    // Read all of payload before processing it.
    if (!payload_segment_.read_up_to_bytes(in, current_frame_.payload_length)) {
      return std::nullopt;
    }

    // Double buffer between payload segment and current frame buffer.
    util::buffer::double_buffer buffers(&payload_segment_.internal_buffer(), &current_frame_.content);

    if (current_frame_.mask_bit) {
      xor_mask(current_frame_.mask_key, buffers.input(), buffers.output());
      buffers.swap();
    }

    for (const std::unique_ptr<extensions::extension>& ext : extensions_) {
      extensions::extension::hook_return result =
          ext->on_inbound_frame_payload(current_frame_, buffers.input(), buffers.output());
      if (result.close.has_value()) {
        should_close.emplace(result.close.value());
        return std::nullopt;
      }
      if (result.transferred_input_to_output) {
        buffers.swap();
      }
    }

    for (const std::unique_ptr<extensions::extension>& ext : extensions_) {
      extensions::extension::hook_return result = ext->on_inbound_frame_complete(current_frame_, buffers.output());
      if (result.close.has_value()) {
        should_close.emplace(result.close.value());
        return std::nullopt;
      }
    }

    // Assure the data is in the frame buffer, not the segment buffer.
    if (!buffers.is_swapped()) {
      buffers.move_and_swap();
    }

    // Set effective opcode so we know what we are expecting for the next frames.
    if (!is_control(current_frame_.type)) {
      if (current_frame_.fin) {
        // End of a fragmented message
        effective_opcode_in_ = std::nullopt;
      } else if (effective_opcode_in_.has_value()) {
        // Middle of a fragmented message, give out continuation frame with effective opcode.
        current_frame_.type = effective_opcode_in_.value();
      } else {
        // Beginning of a fragmented message.
        effective_opcode_in_ = current_frame_.type;
      }
    }

    frame out = std::move(current_frame_);
    current_frame_.content.make_new();
    payload_segment_.reset();
    state_ = parsing_state::header;
    return out;
  }

  // We should never get here.
  return error::websocket::invalid_frame("Corrupted parser state");
}

result<void> frame_parser::serialize_frame(streambuf& output, opcode type, std::string& payload, bool finished) {
  frame_header header{};
  header.fin = finished;
  header.type = type;
  header.mask_bit = destination_ == endpoint::server;

  // Set up buffers for manipulating payload (masking and extensions).
  util::buffer::double_buffer buffers;
  std::copy(payload.begin(), payload.end(), std::ostreambuf_iterator<char>(&buffers.input()));

  // Run extensions
  for (const std::unique_ptr<extensions::extension>& ext : extensions_) {
    extensions::extension::hook_return result = ext->on_outbound_frame(header, buffers.input(), buffers.output());
    if (result.close.has_value()) {
      return error::websocket::serialization_error();
    }
    if (result.transferred_input_to_output) {
      buffers.swap();
    }

    // Loop invariant: at the end of the loop, the payload data is in buffers.input().
  }

  // Start serializing the header.
  byte_array_t header_bytes;

  byte_t& first_byte = header_bytes.emplace_back();
  if (header.fin) {
    first_byte |= static_cast<byte_t>(first_byte_mask::fin);
  }
  if (header.rsv.rsv1) {
    first_byte |= static_cast<byte_t>(first_byte_mask::rsv1);
  }
  if (header.rsv.rsv2) {
    first_byte |= static_cast<byte_t>(first_byte_mask::rsv2);
  }
  if (header.rsv.rsv3) {
    first_byte |= static_cast<byte_t>(first_byte_mask::rsv3);
  }
  first_byte |= static_cast<byte_t>(header.type) & static_cast<byte_t>(first_byte_mask::opcode);

  byte_t& second_byte = header_bytes.emplace_back();
  if (header.mask_bit) {
    second_byte |= static_cast<byte_t>(second_byte_mask::mask);
  }

  header.payload_length = buffers.input().size();
  if (header.payload_length <= static_cast<std::size_t>(payload_constants::max_one_byte)) {
    second_byte |= header.payload_length;
  } else if (is_control(header.type)) {
    return error::websocket::invalid_frame("Control frame payload cannot exceed 125 bytes");
  } else if (header.payload_length <= static_cast<std::size_t>(payload_constants::max_two_byte)) {
    second_byte |= static_cast<byte_t>(payload_constants::payload_length_two_byte);
    util::bytes::insert<2>(std::back_inserter(header_bytes), header.payload_length);
  } else {
    second_byte |= static_cast<byte_t>(payload_constants::payload_length_eight_byte);
    util::bytes::insert<8>(std::back_inserter(header_bytes), header.payload_length);
  }

  // Generate XOR masking key and mask data.
  if (destination_ == endpoint::server) {
    static std::random_device seed;
    static thread_local std::mt19937 generator(seed());
    std::uniform_int_distribution<std::uint32_t> distribution(std::numeric_limits<std::uint32_t>::min(),
                                                              std::numeric_limits<std::uint32_t>::max());
    header.mask_key = distribution(generator);
    xor_mask(header.mask_key, buffers.input(), buffers.output());
    buffers.swap();
    util::bytes::insert<4>(std::back_inserter(header_bytes), header.mask_key);
  }

  // Move data to buffers.output().
  buffers.swap();

  // Write serialized frame out.
  std::copy(header_bytes.begin(), header_bytes.end(), std::ostreambuf_iterator<char>(&output));
  std::copy(std::istreambuf_iterator<char>(&buffers.output()), std::istreambuf_iterator<char>(),
            std::ostreambuf_iterator<char>(&output));
  return util::ok;
}

result<void> frame_parser::serialize(streambuf& output, close_frame&& frame) {
  std::string payload;
  util::bytes::insert<2>(std::back_inserter(payload), static_cast<std::uint16_t>(frame.code));
  if (!frame.reason.empty()) {
    payload += std::move(frame.reason);
  }
  return serialize_frame(output, opcode::close, payload, true);
}

result<void> frame_parser::serialize(streambuf& output, ping_frame&& frame) {
  return serialize_frame(output, opcode::ping, frame.payload, true);
}

result<void> frame_parser::serialize(streambuf& output, pong_frame&& frame) {
  return serialize_frame(output, opcode::pong, frame.payload, true);
}

result<void> frame_parser::serialize(streambuf& output, message_frame&& frame) {
  if (effective_opcode_out_.has_value()) {
    // Continuation of a message.
    if (effective_opcode_out_.value() != frame.type) {
      return error::websocket::unexpected_opcode(
          out::string::stream("Unexpected opcode when serializing frame (expected ",
                              opcode_to_string(effective_opcode_out_.value()).ok(), "; rececived ", frame.type, ")"));
    } else {
      frame.type = opcode::continuation;
    }
  } else {
    // Start of a new message.
    effective_opcode_out_ = frame.type;
  }

  if (frame.finished) {
    // End of a message
    effective_opcode_out_ = std::nullopt;
  }

  return serialize_frame(output, frame.type, frame.payload, frame.finished);
}

}  // namespace proxy::websocket::protocol
