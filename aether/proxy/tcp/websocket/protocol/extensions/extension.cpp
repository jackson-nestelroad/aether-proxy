
#include "extension.hpp"

#include <aether/proxy/tcp/websocket/protocol/extensions/permessage_deflate.hpp>

namespace proxy::tcp::websocket::protocol::extensions {
    extension::~extension() { }

    extension::hook_return extension::on_inbound_frame_header(const frame_header &fh) {
        return { true };
    }
    
    extension::hook_return extension::on_inbound_frame_payload(const frame_header &fh, streambuf &input, streambuf &output) {
        return { false };
    }

    extension::hook_return extension::on_inbound_frame_complete(const frame_header &fh, streambuf &output) {
        return { false };
    }

    extension::hook_return extension::on_outbound_frame(frame_header &fh, streambuf &input, streambuf &output) {
        return { false };
    }

    struct registered_extension_map
        : public std::unordered_map<std::string_view, extension::registered> {
        registered_extension_map() {
#define X(name, str) this->operator[](str) = extension::registered::name;
            WEBSOCKET_EXTENSIONS(X)
#undef X
        }
    };

    std::unique_ptr<extension> extension::from_extension_data(endpoint caller, const handshake::extension_data &data) {
        static registered_extension_map map;
        const auto &it = map.find(data.get_name());
        if (it == map.end()) {
            return nullptr;
        }
        switch (it->second) {
            case registered::permessage_deflate: return std::make_unique<permessage_deflate>(caller, data);
        }
        return nullptr;
    }
}