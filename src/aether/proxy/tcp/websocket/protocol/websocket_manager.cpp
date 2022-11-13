/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "websocket_manager.hpp"

namespace proxy::tcp::websocket::protocol {

websocket_manager::websocket_manager(endpoint ep, const std::vector<handshake::extension_data>& extension_data)
    : frame_parser_(ep, extension_data) {}

std::vector<completed_frame> websocket_manager::parse(streambuf& input, std::optional<close_code>& should_close) {
  std::vector<completed_frame> result;
  while (true) {
    // Loop until parsing hits an error or stops because we've run out of data.
    auto next_frame = frame_parser_.parse(input, should_close);
    if (should_close.has_value() || !next_frame.has_value()) {
      break;
    }

    // Convert frame to its interface type.
    auto& frame = next_frame.value();
    switch (frame.type) {
      case opcode::ping: {
        auto& new_frame = result.emplace_back();
        new_frame.emplace<ping_frame>().payload = frame.content.string_view();
      } break;
      case opcode::pong: {
        auto& new_frame = result.emplace_back();
        new_frame.emplace<pong_frame>().payload = frame.content.string_view();
      } break;
      case opcode::close: {
        auto& new_frame = result.emplace_back();
        process_close_frame(frame, new_frame.emplace<close_frame>());
      } break;
      case opcode::text:
      case opcode::binary: {
        auto& new_frame = result.emplace_back();
        auto& msg_frame = new_frame.emplace<message_frame>();
        msg_frame.finished = frame.fin;
        msg_frame.type = frame.type;
        msg_frame.payload = frame.content.string_view();
      } break;
      default:
        break;
    }
  }
  return result;
}

void websocket_manager::process_close_frame(frame& in, close_frame& out) {
  util::buffer::buffer_segment reader;

  // Need at least two bytes.
  if (!reader.read_up_to_bytes(in.content, 2)) {
    if (reader.bytes_last_read() == 1) {
      throw error::websocket::invalid_frame_exception{"Close frame cannot have 1 byte payload"};
    }
    // No bytes!
    out.code = close_code::no_status_rcvd;
  } else {
    out.code = static_cast<close_code>(util::bytes::parse_network_byte_order<2>(reader.committed_data()));
    // Rest of the data is the reason.
    out.reason = in.content.string_view();
  }
}

void websocket_manager::serialize(streambuf& output, completed_frame&& frame) {
  switch (frame.type()) {
    case opcode::ping:
      frame_parser_.serialize(output, std::move(frame.get_ping_frame()));
      break;
    case opcode::pong:
      frame_parser_.serialize(output, std::move(frame.get_pong_frame()));
      break;
    case opcode::close:
      frame_parser_.serialize(output, std::move(frame.get_close_frame()));
      break;
    case opcode::binary:
    case opcode::text:
      frame_parser_.serialize(output, std::move(frame.get_message_frame()));
      break;
    default:
      throw error::websocket::invalid_opcode_exception{"Invalid opcode to serialize"};
  }
}

}  // namespace proxy::tcp::websocket::protocol
