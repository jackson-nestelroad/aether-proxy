/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <mutex>
#include <optional>
#include <queue>
#include <string_view>
#include <vector>

#include "aether/proxy/tcp/http/exchange.hpp"
#include "aether/proxy/tcp/websocket/handshake/extension_data.hpp"
#include "aether/proxy/tcp/websocket/handshake/handshake.hpp"
#include "aether/proxy/tcp/websocket/message/frame.hpp"
#include "aether/proxy/tcp/websocket/message/message.hpp"

namespace proxy::tcp::websocket {

// Interface for collecting, intercepting, and injecting WebSocket messages across a connection flow.
class pipeline {
 public:
  pipeline(const http::exchange& handshake, bool should_intercept);
  pipeline() = delete;
  ~pipeline() = default;
  pipeline(const pipeline& other) = delete;
  pipeline& operator=(const pipeline& other) = delete;
  pipeline(pipeline&& other) noexcept = delete;
  pipeline& operator=(pipeline&& other) noexcept = delete;

  inline bool should_intercept() const { return should_intercept_; }
  inline void set_interception(bool flag) { should_intercept_ = flag; }
  inline bool closed() { return closed_; }
  inline endpoint closed_by() const { return closed_by_; }
  inline close_code close_code() const { return closed_frame_.code; }
  inline std::string_view close_reason() const { return closed_frame_.reason; }
  inline close_frame get_close_frame() const { return closed_frame_; }

  // Records how the WebSocket pipeline closed and marks the pipeline as closed.
  void set_close_state(endpoint closer, const close_frame& frame);

  inline std::string_view client_key() const { return client_key_; }
  inline std::optional<std::string_view> client_protocol() const { return client_protocol_; }
  inline std::string_view server_accept() const { return server_accept_; }
  inline std::optional<std::string_view> server_protocol() const { return server_protocol_; }
  inline const std::vector<handshake::extension_data>& extensions() const { return extensions_; }

  bool has_message(endpoint caller);
  message& next_message(endpoint caller);
  void pop_message(endpoint caller);
  void inject_message(endpoint destination, const message& msg);

  bool has_frame(endpoint caller);
  completed_frame& next_frame(endpoint caller);
  void pop_frame(endpoint caller);
  void inject_frame(endpoint destination, const completed_frame& frame);

 private:
  constexpr std::mutex& get_mutex_for_endpoint(endpoint ep) {
    return ep == endpoint::client ? client_queue_mutex_ : server_queue_mutex_;
  }

  constexpr std::queue<message>& get_message_queue_for_endpoint(endpoint ep) {
    return ep == endpoint::client ? client_message_queue_ : server_message_queue_;
  }

  constexpr const std::queue<message>& get_message_queue_for_endpoint(endpoint ep) const {
    return ep == endpoint::client ? client_message_queue_ : server_message_queue_;
  }

  constexpr std::queue<completed_frame>& get_frame_queue_for_endpoint(endpoint ep) {
    return ep == endpoint::client ? client_frame_queue_ : server_frame_queue_;
  }

  constexpr const std::queue<completed_frame>& get_frame_queue_for_endpoint(endpoint ep) const {
    return ep == endpoint::client ? client_frame_queue_ : server_frame_queue_;
  }

  bool should_intercept_;
  bool closed_;
  std::mutex closing_state_mutex_;

  endpoint closed_by_;
  close_frame closed_frame_;

  std::string client_key_;
  std::optional<std::string> client_protocol_;
  std::string server_accept_;
  std::optional<std::string> server_protocol_;
  std::vector<handshake::extension_data> extensions_;

  // Mutexes and queues used to inject WebSocket messages into the connection flow.

  std::mutex client_queue_mutex_;
  std::mutex server_queue_mutex_;
  std::queue<message> client_message_queue_;
  std::queue<message> server_message_queue_;
  std::queue<completed_frame> client_frame_queue_;
  std::queue<completed_frame> server_frame_queue_;
};

}  // namespace proxy::tcp::websocket
