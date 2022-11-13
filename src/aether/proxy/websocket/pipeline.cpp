/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "pipeline.hpp"

#include <mutex>

namespace proxy::websocket {
pipeline::pipeline(const http::exchange& handshake, bool should_intercept)
    : should_intercept_(should_intercept), closed_(false), closed_by_(), closed_frame_() {
  client_key_ = handshake::get_client_key(handshake.request());
  client_protocol_ = handshake::get_protocol(handshake.request());
  server_accept_ = handshake::get_server_accept(handshake.response());
  server_protocol_ = handshake::get_protocol(handshake.response());
  extensions_ = handshake::get_extensions(handshake.response());
}

void pipeline::set_close_state(endpoint closer, const close_frame& frame) {
  std::lock_guard<std::mutex> lock(closing_state_mutex_);

  closed_ = true;
  closed_by_ = closer;
  closed_frame_.code = frame.code;
  closed_frame_.reason = frame.reason;
}

bool pipeline::has_message(endpoint caller) {
  std::lock_guard<std::mutex> lock(get_mutex_for_endpoint(caller));
  return !get_message_queue_for_endpoint(caller).empty();
}

message& pipeline::next_message(endpoint caller) {
  std::lock_guard<std::mutex> lock(get_mutex_for_endpoint(caller));
  return get_message_queue_for_endpoint(caller).front();
}

void pipeline::pop_message(endpoint caller) {
  std::lock_guard<std::mutex> lock(get_mutex_for_endpoint(caller));
  get_message_queue_for_endpoint(caller).pop();
}

void pipeline::inject_message(endpoint destination, const message& msg) {
  std::lock_guard<std::mutex> lock(get_mutex_for_endpoint(destination));
  get_message_queue_for_endpoint(destination).push(msg);
}

bool pipeline::has_frame(endpoint caller) {
  std::lock_guard<std::mutex> lock(get_mutex_for_endpoint(caller));
  return !get_frame_queue_for_endpoint(caller).empty();
}

completed_frame& pipeline::next_frame(endpoint caller) {
  std::lock_guard<std::mutex> lock(get_mutex_for_endpoint(caller));
  return get_frame_queue_for_endpoint(caller).front();
}

void pipeline::pop_frame(endpoint caller) {
  std::lock_guard<std::mutex> lock(get_mutex_for_endpoint(caller));
  get_frame_queue_for_endpoint(caller).pop();
}

void pipeline::inject_frame(endpoint destination, const completed_frame& frame) {
  // Injecting a close frame is the same as setting the close state.
  if (frame.type() == opcode::close) {
    set_close_state(~destination, frame.get_close_frame());
  } else {
    std::lock_guard<std::mutex> lock(get_mutex_for_endpoint(destination));
    get_frame_queue_for_endpoint(destination).push(frame);
  }
}

}  // namespace proxy::websocket
