/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "websocket_manager.hpp"

#include "aether/util/result_macros.hpp"

namespace proxy::websocket::protocol {

websocket_manager::websocket_manager(endpoint ep, const std::vector<handshake::extension_data>& extension_data)
    : frame_parser_(ep, extension_data) {}

result<std::vector<completed_frame>> websocket_manager::parse(streambuf& input,
                                                              std::optional<close_code>& should_close) {
  std::vector<completed_frame> result;
  while (true) {
    // Loop until parsing hits an error or stops because we've run out of data.
    ASSIGN_OR_RETURN(auto next_frame, frame_parser_.parse(input, should_close));
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
        RETURN_IF_ERROR(process_close_frame(frame, new_frame.emplace<close_frame>()));
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

result<void> websocket_manager::process_close_frame(frame& in, close_frame& out) {
  util::buffer::buffer_segment reader;

  // Need at least two bytes.
  if (!reader.read_up_to_bytes(in.content, 2)) {
    if (reader.bytes_last_read() == 1) {
      return error::websocket::invalid_frame("Close frame cannot have 1 byte payload");
    }
    // No bytes!
    out.code = close_code::no_status_rcvd;
  } else {
    out.code = static_cast<close_code>(util::bytes::parse_network_byte_order<2>(reader.committed_data()));
    // Rest of the data is the reason.
    out.reason = in.content.string_view();
  }
  return util::ok;
}

result<void> websocket_manager::serialize(streambuf& output, completed_frame&& frame) {
  switch (frame.type()) {
    case opcode::ping:
      return frame_parser_.serialize(output, std::move(frame.get_ping_frame()));
    case opcode::pong:
      return frame_parser_.serialize(output, std::move(frame.get_pong_frame()));
    case opcode::close:
      return frame_parser_.serialize(output, std::move(frame.get_close_frame()));
    case opcode::binary:
    case opcode::text:
      return frame_parser_.serialize(output, std::move(frame.get_message_frame()));
    default:
      return error::websocket::invalid_opcode("Invalid opcode to serialize");
  }
}

}  // namespace proxy::websocket::protocol
