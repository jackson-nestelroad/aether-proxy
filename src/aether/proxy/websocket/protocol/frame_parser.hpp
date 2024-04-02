/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <limits>
#include <memory>
#include <optional>
#include <vector>

#include "aether/proxy/error/error.hpp"
#include "aether/proxy/error/exceptions.hpp"
#include "aether/proxy/websocket/message/endpoint.hpp"
#include "aether/proxy/websocket/message/frame.hpp"
#include "aether/proxy/websocket/protocol/extensions/extension.hpp"
#include "aether/util/buffer_segment.hpp"
#include "aether/util/streambuf.hpp"

namespace proxy::websocket::protocol {

// A stateful parser class for WebSocket frames.
// The parser parses data from an input buffer, returning out new frames as they are read.
class frame_parser {
 public:
  frame_parser(endpoint destination, const std::vector<handshake::extension_data>& extension_data);
  frame_parser() = delete;
  ~frame_parser() = default;
  frame_parser(const frame_parser& other) = delete;
  frame_parser& operator=(const frame_parser& other) = delete;
  frame_parser(frame_parser&& other) noexcept = delete;
  frame_parser& operator=(frame_parser&& other) noexcept = delete;

  // Reads and parses the data in the input buffer.
  // When a frame is completed, it is returned out, and any extra data is left in the input buffer.
  result<std::optional<frame>> parse(streambuf& in, std::optional<close_code>& should_close);

  // Serializes a close frame, placing it in the output buffer.
  result<void> serialize(streambuf& output, close_frame&& frame);

  // Serializes a ping frame, placing it in the output buffer.
  result<void> serialize(streambuf& output, ping_frame&& frame);

  // Serializes a pong frame, placing it in the output buffer.
  result<void> serialize(streambuf& output, pong_frame&& frame);

  // Serializes a message frame, placing it in the output buffer.
  result<void> serialize(streambuf& output, message_frame&& frame);

 private:
  enum class payload_constants : std::uint64_t {
    payload_length_two_byte = 126,
    payload_length_eight_byte = 127,
    max_one_byte = 125,
    max_two_byte = std::numeric_limits<std::uint16_t>::max(),
    max_eight_byte = std::numeric_limits<std::uint64_t>::max()
  };

  enum class first_byte_mask { fin = 1 << 7, rsv1 = 1 << 6, rsv2 = 1 << 5, rsv3 = 1 << 4, opcode = 0xF };

  enum class second_byte_mask { mask = 1 << 7, payload_length = 0x7F };

  enum class parsing_state { header, payload_length, mask_key, payload };

  void xor_mask(std::uint32_t key, std::streambuf& input, std::streambuf& output);
  result<void> serialize_frame(streambuf& output, opcode type, std::string& payload, bool finished);

  endpoint destination_;
  parsing_state state_;
  frame current_frame_;
  std::optional<opcode> effective_opcode_in_;
  std::optional<opcode> effective_opcode_out_;

  std::vector<std::unique_ptr<extensions::extension>> extensions_;

  util::buffer::buffer_segment header_segment_;
  util::buffer::buffer_segment payload_length_segment_;
  util::buffer::buffer_segment mask_key_segment_;
  util::buffer::buffer_segment payload_segment_;
};

}  // namespace proxy::websocket::protocol
