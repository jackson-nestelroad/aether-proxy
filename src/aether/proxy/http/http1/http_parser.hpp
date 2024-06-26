/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>
#include <boost/blank.hpp>
#include <iostream>
#include <utility>

#include "aether/program/options.hpp"
#include "aether/proxy/error/error.hpp"
#include "aether/proxy/http/exchange.hpp"
#include "aether/proxy/types.hpp"
#include "aether/util/buffer_segment.hpp"
#include "aether/util/console.hpp"

namespace proxy {
class server_components;
}

namespace proxy::http::http1 {

// Class for parsing a HTTP/1.x request from an input stream.
//
// Can parse a request and response at the same time.
class http_parser {
 public:
  // Enum for passing in which HTTP message object to send parsed data to.
  enum class message_mode { unknown, request, response };

  // Enumeration type for what type of body size is expected.
  enum class body_size_type {
    // No body at all.
    none,
    // Read given number of bytes.
    given,
    // Read in chunked encoding.
    chunked,
    // Read until the end of the stream.
    all,
  };

  struct body_parsing_state {
    message_mode mode = message_mode::unknown;
    body_size_type type = body_size_type::none;
    std::size_t expected_size = 0;
    std::size_t read = 0;
    bool finished = false;
    bool next_chunk_size_known = false;
  };

  http_parser(exchange& exch, server_components& components);
  http_parser() = delete;
  ~http_parser() = default;
  http_parser(const http_parser& parser) = delete;
  http_parser& operator=(const http_parser& parser) = delete;
  http_parser(http_parser&& parser) = delete;
  http_parser& operator=(http_parser&& parser) = delete;

  // Parses the request line from the stream.
  result<void> read_request_line(std::istream& in);

  // Parses the response line from the stream.
  result<void> read_response_line(std::istream& in);

  // Reads the headers from the stream.
  result<void> read_headers(std::istream& in, message_mode mode);

  // Reads the message body from the stream.
  //
  // This method is stateful, and it returns a boolean indicating if the read was complete or not.
  //
  // State is automatically reset when the body is completely read.
  //
  // Do not switch message mode between reads.
  result<bool> read_body(std::istream& in, message_mode mode);

 private:
  // Gets the reference for where to save the parsed data.
  result<message*> get_data_for_mode(message_mode mode);

  // Checks the expected body size based on the request and response data currently in the parser.
  result<std::pair<body_size_type, std::size_t>> expected_body_size(message_mode mode);

  // Resets the body parsing status.
  inline void reset_body_parsing_state() { state_ = {}; }

  program::options& options_;

  // The data the parser writes to is managed by an exchange.
  exchange& exchange_;

  // Internal state for parsing a HTTP body, since it can span multiple calls.
  body_parsing_state state_;

  // Buffer segments for managing compound reads.

  util::buffer::buffer_segment request_method_buf_;
  util::buffer::buffer_segment request_target_buf_;
  util::buffer::buffer_segment request_version_buf_;
  util::buffer::buffer_segment response_version_buf_;
  util::buffer::buffer_segment response_code_buf_;
  util::buffer::buffer_segment response_msg_buf_;
  util::buffer::buffer_segment header_buf_;
  util::buffer::buffer_segment chunk_header_buf_;
  util::buffer::buffer_segment chunk_suffix_buf_;
  util::buffer::buffer_segment body_buf_;
};

}  // namespace proxy::http::http1
