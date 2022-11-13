/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "extension.hpp"

#include <memory>
#include <string_view>
#include <unordered_map>

#include "aether/proxy/tcp/websocket/message/endpoint.hpp"
#include "aether/proxy/tcp/websocket/message/frame.hpp"
#include "aether/proxy/tcp/websocket/protocol/extensions/permessage_deflate.hpp"
#include "aether/util/streambuf.hpp"

namespace proxy::tcp::websocket::protocol::extensions {

extension::~extension() {}

extension::hook_return extension::on_inbound_frame_header(const frame_header& fh) { return {true}; }

extension::hook_return extension::on_inbound_frame_payload(const frame_header& fh, streambuf& input,
                                                           streambuf& output) {
  return {false};
}

extension::hook_return extension::on_inbound_frame_complete(const frame_header& fh, streambuf& output) {
  return {false};
}

extension::hook_return extension::on_outbound_frame(frame_header& fh, streambuf& input, streambuf& output) {
  return {false};
}

struct registered_extension_map : public std::unordered_map<std::string_view, extension::registered> {
  registered_extension_map() {
#define X(name, str) this->operator[](str) = extension::registered::name;
    WEBSOCKET_EXTENSIONS(X)
#undef X
  }
};

std::unique_ptr<extension> extension::from_extension_data(endpoint caller, const handshake::extension_data& data) {
  static registered_extension_map map;
  const auto& it = map.find(data.name());
  if (it == map.end()) {
    return nullptr;
  }
  switch (it->second) {
    case registered::permessage_deflate:
      return std::make_unique<permessage_deflate>(caller, data);
    default:
      break;
  }
  return nullptr;
}

}  // namespace proxy::tcp::websocket::protocol::extensions
