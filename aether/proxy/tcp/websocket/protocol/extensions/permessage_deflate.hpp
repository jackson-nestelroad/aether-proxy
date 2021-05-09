/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <array>
#include <zlib.h>

#include <aether/proxy/tcp/websocket/protocol/extensions/extension.hpp>
#include <aether/util/console.hpp>

namespace proxy::tcp::websocket::protocol::extensions {
    /*
        WebSocket extension implementation for per-message deflation.
        Decompresses inbound frames and compresses outbound frames using zlib.
    */
    class permessage_deflate
        : public extension
    {
    private:
        static constexpr std::array<proxy::byte_t, 4> flush_marker = { 0x00, 0x00, 0xFF, 0xFF };
        static constexpr std::array<proxy::byte_t, 6> empty_content = { 0x02, 0x00, 0x00, 0x00, 0xFF, 0xFF };
        static constexpr int default_max_window_bits = 15;
        static constexpr int buffer_size = 8192;

        bool client_no_context_takeover;
        bool server_no_context_takeover;
        int client_max_window_bits;
        int server_max_window_bits;
        int flush;

        std::optional<bool> inbound_is_compressible;
        std::optional<bool> inbound_compressed;

        z_stream deflate_stream;
        z_stream inflate_stream;

        bool opcode_is_compressible(opcode type) const;

    public:
        permessage_deflate(endpoint caller, const handshake::extension_data &data);
        ~permessage_deflate();

        hook_return on_inbound_frame_header(const frame_header &fh) override;
        hook_return on_inbound_frame_payload(const frame_header &fh, streambuf &input, streambuf &output) override;
        hook_return on_inbound_frame_complete( const frame_header &fh, streambuf &output) override;
        hook_return on_outbound_frame(frame_header &fh, streambuf &input, streambuf &output) override;
    };
}