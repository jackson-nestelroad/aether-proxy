/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <zlib.h>

#include <array>
#include <memory>
#include <optional>

#include "aether/proxy/error/error.hpp"
#include "aether/proxy/types.hpp"
#include "aether/proxy/websocket/message/frame.hpp"
#include "aether/proxy/websocket/protocol/extensions/extension.hpp"

namespace proxy::websocket::protocol::extensions {

// WebSocket extension implementation for per-message deflation.
// Decompresses inbound frames and compresses outbound frames using zlib.
class permessage_deflate : public extension {
 public:
  static result<std::unique_ptr<extension>> create(endpoint caller, const handshake::extension_data& data);

  ~permessage_deflate();

  hook_return on_inbound_frame_header(const frame_header& fh) override;
  hook_return on_inbound_frame_payload(const frame_header& fh, streambuf& input, streambuf& output) override;
  hook_return on_inbound_frame_complete(const frame_header& fh, streambuf& output) override;
  hook_return on_outbound_frame(frame_header& fh, streambuf& input, streambuf& output) override;

 private:
  static constexpr std::array<byte_t, 4> flush_marker = {0x00, 0x00, 0xFF, 0xFF};
  static constexpr std::array<byte_t, 6> empty_content = {0x02, 0x00, 0x00, 0x00, 0xFF, 0xFF};
  static constexpr int default_max_window_bits = 15;
  static constexpr int buffer_size = 8192;

  permessage_deflate(endpoint caller, const handshake::extension_data& data);

  bool opcode_is_compressible(opcode type) const;

  bool client_no_context_takeover_;
  bool server_no_context_takeover_;
  int client_max_window_bits_;
  int server_max_window_bits_;
  int flush_;

  std::optional<bool> inbound_is_compressible_;
  std::optional<bool> inbound_compressed_;

  z_stream deflate_stream_;
  z_stream inflate_stream_;
};

}  // namespace proxy::websocket::protocol::extensions
