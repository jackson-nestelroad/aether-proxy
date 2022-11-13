/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "permessage_deflate.hpp"

#include <zlib.h>

#include <algorithm>
#include <boost/lexical_cast.hpp>

#include "aether/proxy/error/exceptions.hpp"
#include "aether/proxy/types.hpp"
#include "aether/proxy/websocket/message/frame.hpp"
#include "aether/proxy/websocket/protocol/extensions/extension.hpp"
#include "aether/util/console.hpp"

namespace proxy::websocket::protocol::extensions {

permessage_deflate::permessage_deflate(endpoint caller, const handshake::extension_data& data) {
  client_no_context_takeover_ = data.has_param("client_no_context_takeover");
  server_no_context_takeover_ = data.has_param("server_no_context_takeover");

  client_max_window_bits_ = data.has_param("client_max_window_bits")
                                ? boost::lexical_cast<int>(data.get_param("client_max_window_bits"))
                                : default_max_window_bits;

  server_max_window_bits_ = data.has_param("server_max_window_bits")
                                ? boost::lexical_cast<int>(data.get_param("server_max_window_bits"))
                                : default_max_window_bits;

  int deflate_bits, inflate_bits;
  if (caller == endpoint::client) {
    deflate_bits = client_max_window_bits_;
    inflate_bits = server_max_window_bits_;
  } else {
    deflate_bits = server_max_window_bits_;
    inflate_bits = client_max_window_bits_;
  }

  deflate_stream_.zalloc = Z_NULL;
  deflate_stream_.zfree = Z_NULL;
  deflate_stream_.opaque = Z_NULL;

  inflate_stream_.zalloc = Z_NULL;
  inflate_stream_.zfree = Z_NULL;
  inflate_stream_.opaque = Z_NULL;

  int res = deflateInit2(&deflate_stream_, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -1 * deflate_bits, 4, Z_DEFAULT_STRATEGY);
  if (res != Z_OK) {
    throw error::websocket::zlib_error_exception{"Could not initialize deflate stream"};
  }

  res = inflateInit2(&inflate_stream_, -1 * inflate_bits);
  if (res != Z_OK) {
    throw error::websocket::zlib_error_exception{"Could not initialize inflate stream"};
  }

  if ((client_no_context_takeover_ && caller == endpoint::client) ||
      (server_no_context_takeover_ && caller == endpoint::server)) {
    flush_ = Z_FULL_FLUSH;
  } else {
    flush_ = Z_SYNC_FLUSH;
  }

  inbound_is_compressible_ = false;
  inbound_compressed_ = {};
}

permessage_deflate::~permessage_deflate() {
  // No need to check error codes because we never call Z_FINISH.
  deflateEnd(&deflate_stream_);
  inflateEnd(&inflate_stream_);
}

bool permessage_deflate::opcode_is_compressible(opcode type) const { return !is_control(type); }

extension::hook_return permessage_deflate::on_inbound_frame_header(const frame_header& fh) {
  // RSV1 bit is set on first compressed frame.
  if (fh.rsv.rsv1 && (is_control(fh.type) || fh.type == opcode::continuation)) {
    return {false, close_code::protocol_error};
  }

  // Non-control frames are compressible.
  inbound_is_compressible_ = opcode_is_compressible(fh.type);

  // Flag for if the current inbound message is compressed.
  // This flag stays set for continuation frames.
  if (!inbound_compressed_.has_value()) {
    inbound_compressed_ = fh.rsv.rsv1;
  }

  return {false};
}

extension::hook_return permessage_deflate::on_inbound_frame_payload(const frame_header& fh, streambuf& input,
                                                                    streambuf& output) {
  if (!inbound_compressed_.value_or(false) || !inbound_is_compressible_.value_or(false)) {
    return {false};
  }

  // Append tail.
  std::copy(flush_marker.begin(), flush_marker.end(), std::ostreambuf_iterator<char>(&input));

  // Decompress input stream.
  auto data = input.mutable_input();
  inflate_stream_.avail_in = static_cast<int>(data.size());
  inflate_stream_.next_in = reinterpret_cast<byte_t*>(data.data());

  do {
    auto buffer = output.prepare(buffer_size);
    inflate_stream_.avail_out = buffer_size;
    inflate_stream_.next_out = reinterpret_cast<byte_t*>(buffer.data());

    int ret = inflate(&inflate_stream_, Z_SYNC_FLUSH);
    if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
      out::safe_error::log(inflate_stream_.msg);
      return {false, close_code::invalid_frame_payload_data};
    }

    output.commit(static_cast<std::size_t>(buffer_size) - inflate_stream_.avail_out);
  } while (inflate_stream_.avail_out == 0);

  input.consume(data.size());
  return {true};
}

extension::hook_return permessage_deflate::on_inbound_frame_complete(const frame_header& fh, streambuf& output) {
  // Message not finished, keep internal state.
  if (!fh.fin) {
    return {false};
  }

  // Inbound message is finished and not compressed, reset state.
  if (!inbound_is_compressible_.value_or(false)) {
    inbound_is_compressible_ = {};
    return {false};
  }

  // Next frame may or may not be compressed.
  // Control frames, which are not compressed, can interrupt message frames.
  inbound_compressed_ = {};
  return {false};
}

extension::hook_return permessage_deflate::on_outbound_frame(frame_header& fh, streambuf& input, streambuf& output) {
  if (!opcode_is_compressible(fh.type)) {
    return {false};
  }

  if (fh.type != opcode::continuation) {
    fh.rsv.rsv1 = true;
  }

  if (input.size() == 0) {
    std::copy(empty_content.begin(), empty_content.end(), std::ostreambuf_iterator<char>(&output));
    return {true};
  }

  // This logic is a bit tricky because the WebSocket protocol requires the compressed message to be sent without the
  // ending flush marker

  // However, it is difficult to tell exactly where it is, especially if the message is exactly as long as the buffer we
  // use

  // The message is compressed in two parts:
  //  1. Compress using Z_BLOCK until the buffer is no longer filled by a deflate call.
  //  2. Call deflate again, flushing the output (which is at most 7 bits, plus the flush marker). We know the flush
  //  marker is appended during this final call, so we just ignore those final bytes.

  auto data = input.mutable_input();
  deflate_stream_.avail_in = static_cast<int>(data.size());
  deflate_stream_.next_in = reinterpret_cast<byte_t*>(data.data());

  do {
    auto buffer = output.prepare(buffer_size);
    deflate_stream_.avail_out = buffer_size;
    deflate_stream_.next_out = reinterpret_cast<byte_t*>(buffer.data());

    int ret = deflate(&deflate_stream_, Z_BLOCK);
    if (ret != Z_OK) {
      return {false, close_code::invalid_frame_payload_data};
    }

    output.commit(static_cast<std::size_t>(buffer_size) - deflate_stream_.avail_out);
  } while (deflate_stream_.avail_out == 0);

  deflate_stream_.avail_in = 0;
  deflate_stream_.next_in = Z_NULL;
  std::size_t last_buffer_size = flush_marker.size() + 2;
  auto buffer = output.prepare(last_buffer_size);
  deflate_stream_.avail_out = static_cast<int>(last_buffer_size);
  deflate_stream_.next_out = reinterpret_cast<byte_t*>(buffer.data());
  int ret = deflate(&deflate_stream_, flush_);
  if (ret != Z_OK) {
    return {false, close_code::invalid_frame_payload_data};
  }
  output.commit(2LL - deflate_stream_.avail_out);

  input.consume(data.size());
  return {true};
}

}  // namespace proxy::websocket::protocol::extensions
