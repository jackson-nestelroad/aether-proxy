/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <string>
#include <string_view>

#include "aether/proxy/tcp/websocket/message/endpoint.hpp"
#include "aether/proxy/tcp/websocket/message/opcode.hpp"

namespace proxy::tcp::websocket {

// Represents a finished WebSocket message, which is a collection of frames.
class message {
 public:
  message(opcode type, endpoint origin);
  message(opcode type, endpoint origin, std::string_view content);
  message(endpoint origin, std::string_view content);
  message() = delete;
  ~message() = default;
  message(const message& other) = default;
  message& operator=(const message& other) = default;
  message(message&& other) noexcept = default;
  message& operator=(message&& other) noexcept = default;

  inline opcode type() const { return type_; }
  inline endpoint origin() const { return origin_; }
  std::string_view content() const { return content_; }
  inline void set_content(std::string_view str) { content_ = str; }
  inline void set_content(std::string&& str) { content_ = std::move(str); }
  inline bool blocked() const { return blocked_; }
  inline void block() { blocked_ = true; }
  inline std::size_t size() const { return content_.size(); }

 private:
  opcode type_;
  endpoint origin_;
  std::string content_;
  bool blocked_;
};

}  // namespace proxy::tcp::websocket
