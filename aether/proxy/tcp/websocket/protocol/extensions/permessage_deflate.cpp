/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "permessage_deflate.hpp"

namespace proxy::tcp::websocket::protocol::extensions {
    permessage_deflate::permessage_deflate(endpoint caller, const handshake::extension_data &data) {
        client_no_context_takeover = data.has_param("client_no_context_takeover");
        server_no_context_takeover = data.has_param("server_no_context_takeover");

        std::string window_bits_param_name = caller == endpoint::client ? "client_max_window_bits" : "server_max_window_bits";

        client_max_window_bits = data.has_param("client_max_window_bits")
            ? boost::lexical_cast<int>(data.get_param("client_max_window_bits"))
            : default_max_window_bits;

        server_max_window_bits = data.has_param("server_max_window_bits")
            ? boost::lexical_cast<int>(data.get_param("server_max_window_bits"))
            : default_max_window_bits;

        int deflate_bits, inflate_bits;
        if (caller == endpoint::client) {
            deflate_bits = client_max_window_bits;
            inflate_bits = server_max_window_bits;
        }
        else {
            deflate_bits = server_max_window_bits;
            inflate_bits = client_max_window_bits;
        }

        deflate_stream.zalloc = Z_NULL;
        deflate_stream.zfree = Z_NULL;
        deflate_stream.opaque = Z_NULL;

        inflate_stream.zalloc = Z_NULL;
        inflate_stream.zfree = Z_NULL;
        inflate_stream.opaque = Z_NULL;

        int res = deflateInit2(
            &deflate_stream,
            Z_DEFAULT_COMPRESSION,
            Z_DEFLATED,
            -1 * deflate_bits,
            4,
            Z_DEFAULT_STRATEGY
        );
        if (res != Z_OK) {
            throw error::websocket::zlib_error_exception { "Could not initialize deflate stream" };
        }

        res = inflateInit2(
            &inflate_stream,
            -1 * inflate_bits
        );
        if (res != Z_OK) {
            throw error::websocket::zlib_error_exception { "Could not initialize inflate stream" };
        }

        if ((client_no_context_takeover && caller == endpoint::client) || (server_no_context_takeover && caller == endpoint::server)) {
            flush = Z_FULL_FLUSH;
        }
        else {
            flush = Z_SYNC_FLUSH;
        }

        inbound_is_compressible = false;
        inbound_compressed = { };
    }

    permessage_deflate::~permessage_deflate() {
        // No need to check error codes because we never call Z_FINISH
        deflateEnd(&deflate_stream);
        inflateEnd(&inflate_stream);
    }

    bool permessage_deflate::opcode_is_compressible(opcode type) const {
        return !is_control(type);
    }

    extension::hook_return permessage_deflate::on_inbound_frame_header(const frame_header &fh) {
        // RSV1 bit is set on first compressed frame
        if (fh.rsv.rsv1 && (is_control(fh.type) || fh.type == opcode::continuation)) {
            return { false, close_code::protocol_error };
        }

        // Non-control frames are compressible
        inbound_is_compressible = opcode_is_compressible(fh.type);

        // Flag for if the current inbound message is compressed
        // This flag stays set for continuation frames
        if (!inbound_compressed.has_value()) {
            inbound_compressed = fh.rsv.rsv1;
        }

        return { false };
    }

    extension::hook_return permessage_deflate::on_inbound_frame_payload(const frame_header &fh, streambuf &input, streambuf &output) {
        if (!inbound_compressed.value_or(false) || !inbound_is_compressible.value_or(false)) {
            return { false };
        }

        // Append tail
        std::copy(flush_marker.begin(), flush_marker.end(), std::ostreambuf_iterator<char>(&input));

        // Decompress input stream
        auto data = input.mutable_input();
        inflate_stream.avail_in = static_cast<int>(data.size());
        inflate_stream.next_in = reinterpret_cast<byte_t *>(data.data());

        do {
            auto buffer = output.prepare(buffer_size);
            inflate_stream.avail_out = buffer_size;
            inflate_stream.next_out = reinterpret_cast<byte_t *>(buffer.data());
            
            int ret = inflate(&inflate_stream, Z_SYNC_FLUSH);
            if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
                out::safe_error::log(inflate_stream.msg);
                return { false, close_code::invalid_frame_payload_data };
            }

            output.commit(static_cast<std::size_t>(buffer_size) - inflate_stream.avail_out);
        } while (inflate_stream.avail_out == 0);

        input.consume(data.size());
        return { true };
    }

    extension::hook_return permessage_deflate::on_inbound_frame_complete(const frame_header &fh, streambuf &output) {
        // Message not finished, keep internal state
        if (!fh.fin) {
            return { false };
        }

        // Inbound message is finished and not compressed, reset state
        if (!inbound_is_compressible.value_or(false)) {
            inbound_is_compressible = { };
            return { false };
        }

        // Next frame may or may not be compressed
        // Control frames, which are not compressed, can interrupt message frames
        inbound_compressed = { };
        return { false };
    }

    extension::hook_return permessage_deflate::on_outbound_frame(frame_header &fh, streambuf &input, streambuf &output) {
        if (!opcode_is_compressible(fh.type)) {
            return { false };
        }

        if (fh.type != opcode::continuation) {
            fh.rsv.rsv1 = true;
        }

        if (input.size() == 0) {
            std::copy(empty_content.begin(), empty_content.end(), std::ostreambuf_iterator<char>(&output));
            return { true };
        }

        // This logic is a bit tricky because the WebSocket protocol requires
        // the compressed message to be sent without the ending flush marker

        // However, it is difficult to tell exactly where it is, especially if the
        // message is exactly as long as the buffer we use

        // The message is compressed in two parts
        // 1. Compress using Z_BLOCK until the buffer is no longer filled by a deflate call
        // 2. Call deflate again, flushing the output (which is at most 7 bits, plus the flush marker)
        // We know the flush marker is appended during this final call, so we just ignore those final bytes

        auto data = input.mutable_input();
        deflate_stream.avail_in = static_cast<int>(data.size());
        deflate_stream.next_in = reinterpret_cast<byte_t *>(data.data());

        do {
            auto buffer = output.prepare(buffer_size);
            deflate_stream.avail_out = buffer_size;
            deflate_stream.next_out = reinterpret_cast<byte_t *>(buffer.data());

            int ret = deflate(&deflate_stream, Z_BLOCK);
            if (ret != Z_OK) {
                return { false, close_code::invalid_frame_payload_data };
            }

            output.commit(static_cast<std::size_t>(buffer_size) - deflate_stream.avail_out);
        } while (deflate_stream.avail_out == 0);

        deflate_stream.avail_in = 0;
        deflate_stream.next_in = Z_NULL;
        std::size_t last_buffer_size = flush_marker.size() + 2;
        auto &&buffer = output.prepare(last_buffer_size);
        deflate_stream.avail_out = static_cast<int>(last_buffer_size);
        deflate_stream.next_out = reinterpret_cast<byte_t *>(buffer.data());
        int ret = deflate(&deflate_stream, flush);
        if (ret != Z_OK) {
            return { false, close_code::invalid_frame_payload_data };
        }
        output.commit(2LL - deflate_stream.avail_out);

        input.consume(data.size());
        return { true };
    }
}