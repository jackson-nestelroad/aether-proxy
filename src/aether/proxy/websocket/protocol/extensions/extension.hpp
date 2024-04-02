/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <optional>
#include <string>
#include <tuple>

#include "aether/proxy/error/error.hpp"
#include "aether/proxy/types.hpp"
#include "aether/proxy/websocket/handshake/extension_data.hpp"
#include "aether/proxy/websocket/message/close_code.hpp"
#include "aether/proxy/websocket/message/endpoint.hpp"
#include "aether/proxy/websocket/message/frame.hpp"
#include "aether/util/streambuf.hpp"

// See https://www.iana.org/assignments/websocket/websocket.xml#extension-name.
#define WEBSOCKET_EXTENSIONS(X)               \
  X(permessage_deflate, "permessage-deflate") \
  X(bbf_usp_protocol, "bbf-usp-protocol")

namespace proxy::websocket::protocol::extensions {

// Represents a single WebSocket extension for frame input/output.
// All methods should be overridden as needed for extension functionality.
// Does nothing by default.
class extension {
 public:
  // Data structure returned from extension event hooks.
  struct hook_return {
    bool transferred_input_to_output;
    std::optional<close_code> close;
  };

  enum class registered : std::uint8_t {
#define X(name, str) name,
    WEBSOCKET_EXTENSIONS(X)
#undef X
        other = static_cast<std::uint8_t>(-1),
  };

  extension() = default;
  virtual ~extension();
  extension(const extension& other) = delete;
  extension& operator=(const extension& other) = delete;
  extension(extension&& other) noexcept = delete;
  extension& operator=(extension&& other) noexcept = delete;

  // WebSocket frame parsing event hooks.

  // Event hook for reading an inbound frame header.
  // Data in the input stream must be transferred to the output stream.
  virtual hook_return on_inbound_frame_header(const frame_header& fh);

  // Event hook for reading an inbound frame payload.
  // Data in the input stream must be transferred to the output stream.
  virtual hook_return on_inbound_frame_payload(const frame_header& fh, streambuf& input, streambuf& output);

  // Event hook for an inbound frame's completion.
  // Any data written to the output stream is appended to the frame.
  virtual hook_return on_inbound_frame_complete(const frame_header& fh, streambuf& output);

  // Event hook for writing an outbound frame.
  // Data in the input stream must be transferred to the output stream.
  virtual hook_return on_outbound_frame(frame_header& fh, streambuf& input, streambuf& output);

  // Creates a WebSocket extension instance from a given instance of extension_data.
  static result<std::unique_ptr<extension>> from_extension_data(endpoint caller, const handshake::extension_data& data);
};

}  // namespace proxy::websocket::protocol::extensions
