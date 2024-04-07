/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "http_parser.hpp"

#include "aether/program/options.hpp"
#include "aether/proxy/error/error.hpp"
#include "aether/proxy/http/exchange.hpp"
#include "aether/proxy/server_components.hpp"
#include "aether/proxy/types.hpp"
#include "aether/util/buffer_segment.hpp"
#include "aether/util/console.hpp"
#include "aether/util/generic_error.hpp"
#include "aether/util/result.hpp"
#include "aether/util/result_macros.hpp"

namespace proxy::http::http1 {

http_parser::http_parser(exchange& exch, server_components& components)
    : options_(components.options), exchange_(exch) {}

namespace {

result<void> assert_not_unknown(http_parser::message_mode mode) {
  if (mode == http_parser::message_mode::unknown) {
    return error::parser_error("Cannot parse data for unknown mode");
  }
  return util::ok;
}

}  // namespace

result<message*> http_parser::get_data_for_mode(message_mode mode) {
  RETURN_IF_ERROR(assert_not_unknown(mode));
  if (mode == message_mode::request) {
    return &exchange_.request();
  }
  return &exchange_.response();
}

result<void> http_parser::read_request_line(std::istream& in) {
  if (!request_method_buf_.read_until(in, message::SP) || !request_target_buf_.read_until(in, message::SP) ||
      !request_version_buf_.read_until(in, message::CRLF)) {
    return error::http::invalid_request_line("Could not read request line");
  }

  // Exceptions will propogate.
  request& req = exchange_.request();
  ASSIGN_OR_RETURN(method verb, string_to_method(request_method_buf_.string_view()));
  req.set_method(verb);
  ASSIGN_OR_RETURN(version vers, string_to_version(request_version_buf_.string_view()));
  req.set_version(vers);
  ASSIGN_OR_RETURN(url target, url::parse_target(request_target_buf_.string_view(), verb));
  req.set_target(target);

  request_method_buf_.reset();
  request_target_buf_.reset();
  request_version_buf_.reset();

  return util::ok;
}

result<void> http_parser::read_response_line(std::istream& in) {
  if (!response_version_buf_.read_until(in, message::SP) || !response_code_buf_.read_until(in, message::SP) ||
      !response_msg_buf_.read_until(in, message::CRLF)) {
    return error::http::invalid_response_line("Could not read response line");
  }

  // Exceptions will propogate.
  ASSIGN_OR_RETURN(version vers, string_to_version(response_version_buf_.string_view()));
  exchange_.response().set_version(vers);
  exchange_.response().set_status(string_to_status(response_code_buf_.string_view()));
  // Message is discarded, we generate it ourselves when we need it.

  response_version_buf_.reset();
  response_code_buf_.reset();
  response_msg_buf_.reset();

  return util::ok;
}

result<void> http_parser::read_headers(std::istream& in, message_mode mode) {
  ASSIGN_OR_RETURN(message* msg, get_data_for_mode(mode));
  while (true) {
    if (!header_buf_.read_until(in, message::CRLF)) {
      return error::http::invalid_header("Error when reading header");
    }

    auto next_line = header_buf_.string_view();

    if (next_line.empty()) {
      header_buf_.reset();
      break;
    }
    std::size_t delim = next_line.find(':');
    if (delim == std::string::npos) {
      return error::http::invalid_header(out::string::stream("No value set for header \"", next_line, "\""));
    }
    std::string_view name = next_line.substr(0, delim);
    std::string_view value = next_line.substr(delim + 1);
    value = util::string::trim(value);
    msg->add_header(name, value);

    header_buf_.reset();
  }
  return util::ok;
}

result<std::pair<http_parser::body_size_type, std::size_t>> http_parser::expected_body_size(message_mode mode) {
  static constexpr std::pair<http_parser::body_size_type, std::size_t> none = {body_size_type::none, 0};
  bool for_request = mode == message_mode::request;
  request& req = exchange_.request();
  if (for_request) {
    if (result<std::string_view> header = req.get_header("Expect"); header.is_ok() && header.ok() == "100-continue") {
      return none;
    }
  } else if (exchange_.has_response()) {
    response& res = exchange_.response();
    if (req.method() == method::HEAD) {
      return none;
    }
    if (res.is_1xx()) {
      return none;
    }
    if (res.status() == status::no_content) {
      return none;
    }
    if (res.status() == status::not_modified) {
      return none;
    }
    if (res.status() == status::ok && req.method() == method::CONNECT) {
      return none;
    }
  }
  ASSIGN_OR_RETURN(message* msg, get_data_for_mode(mode));

  if (msg->header_has_token("Transfer-Encoding", "chunked")) {
    return {body_size_type::chunked, 0};
  }

  if (msg->has_header("Content-Length")) {
    auto sizes = msg->get_all_of_header("Content-Length");
    bool different_sizes = std::adjacent_find(sizes.begin(), sizes.end(), std::not_equal_to<>()) != sizes.end();
    if (different_sizes) {
      return error::http::invalid_body_size("Conflicting Content-Length headers");
    }
    try {
      long long size = boost::lexical_cast<long long>(sizes[0]);
      if (size < 0) {
        throw boost::bad_lexical_cast{};
      }
      return {body_size_type::given, static_cast<std::size_t>(size)};
    } catch (const boost::bad_lexical_cast&) {
      return error::http::invalid_body_size("Invalid Content-Length value");
    }
  }

  // Default cases
  if (for_request) {
    return none;
  }
  return {body_size_type::all, 0};
}

result<bool> http_parser::read_body(std::istream& in, message_mode mode) {
  // Initial read, set up state.
  if (state_.mode == message_mode::unknown) {
    ASSIGN_OR_RETURN((std::pair<body_size_type, std::size_t> pair), expected_body_size(mode));
    // No body to read at all, already successful.
    if (pair.first == body_size_type::none) {
      return true;
    }

    std::size_t body_size_limit = options_.body_size_limit;
    if (pair.second > body_size_limit) {
      return error::http::body_size_too_large();
    } else if (pair.first == body_size_type::all) {
      pair.second = body_size_limit;
    }
    // We are good to go.
    state_ = {mode, pair.first, pair.second, 0};
  }

  // Use buffer::read_until because it will place read characters back into the stream if it fails to find the
  // delimiter. This assures we can return out, read more data from the socket into the stream, and resume reading with
  // the correct amount of information.

  if (state_.type == body_size_type::chunked) {
    while (true) {
      // Need to read chunk header.
      if (!state_.next_chunk_size_known) {
        // Could not read chunk header.
        //
        // Return out to let the socket read again.
        if (!chunk_header_buf_.read_until(in, message::CRLF)) {
          break;
        }

        std::string_view line = chunk_header_buf_.string_view();

        util::result<std::size_t, util::generic_error> res = util::string::parse_hexadecimal(line);
        if (res.is_err()) {
          return error::http::invalid_chunked_body();
        }
        state_.expected_size = res.ok();

        chunk_header_buf_.reset();

        // Going to exceed the limit.
        std::size_t body_size_limit = options_.body_size_limit;
        if (state_.read + state_.expected_size > body_size_limit) {
          return error::http::body_size_too_large();
        }

        state_.next_chunk_size_known = true;
      }

      // Need more data from the socket.
      //
      // If there is no more to read, the service will cancel the request.
      if (!body_buf_.read_up_to_bytes(in, state_.expected_size)) {
        break;
      }

      // Read all of this chunk.
      //
      // Remove suffix, which is a trailing CRLF.
      if (!chunk_suffix_buf_.read_until(in, message::CRLF)) {
        // Could not find suffix, return out to hopefully get more data from the socket.
        break;
      }

      std::string_view line = chunk_suffix_buf_.string_view();

      // Found an invalid suffix.
      if (!line.empty()) {
        return error::http::invalid_chunked_body();
      } else {
        // Everything was successful, reset fields.
        chunk_suffix_buf_.reset();
        // This was the last chunk
        if (state_.expected_size == 0) {
          state_.finished = true;
          break;
        } else {
          // Reset to 0 to read another chunk.
          state_.read += state_.expected_size;
          state_.next_chunk_size_known = false;
          state_.expected_size = 0;
          // Keep the chunk in the body buffer, but allow the next chunk to be read.
          body_buf_.mark_as_incomplete();
        }
      }
    }
  } else if (state_.type == body_size_type::given) {
    // Read all of body into buffer.
    if (body_buf_.read_up_to_bytes(in, state_.expected_size)) {
      state_.read = state_.expected_size;
      state_.finished = true;
    }
  } else {
    // Read until EOF, which is rare but allowed by the standard.
    body_buf_.read_all(in);
    body_buf_.mark_as_incomplete();

    std::size_t just_read = body_buf_.bytes_last_read();
    state_.read += just_read;
    if (state_.read > options_.body_size_limit) {
      return error::http::body_size_too_large();
    }

    if (just_read == 0) {
      state_.finished = true;
    }
  }

  // Reset data when finished.
  if (state_.finished) {
    ASSIGN_OR_RETURN(message* msg, get_data_for_mode(mode));
    msg->set_body(std::string(body_buf_.string_view()));
    body_buf_.reset();
    reset_body_parsing_state();
    return true;
  }

  return false;
}

}  // namespace proxy::http::http1
