/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>
#include <boost/blank.hpp>
#include <boost/noncopyable.hpp>

#include <aether/proxy/types.hpp>
#include <aether/proxy/tcp/http/exchange.hpp>
#include <aether/util/buffer_segment.hpp>
#include <aether/util/console.hpp>

namespace proxy::tcp::http::http1 {
    /*
        Class for parsing a HTTP/1.x request from an input stream.
        Can parse a request and response at the same time.
    */
    class http_parser {
    public:
        /*
            Enum for passing in which HTTP message object to send parsed data to.
        */
        enum class message_mode {
            unknown,
            request,
            response
        };

        /*
            Enumeration type for what type of body size is expected.
        */
        enum class body_size_type {
            // No body at all
            none,
            // Read given number of bytes
            given,
            // Read in chunked encoding
            chunked,
            // Read until the end of the stream
            all
        };

        struct body_parsing_status {
            message_mode mode = message_mode::unknown;
            body_size_type type = body_size_type::none;
            std::size_t expected_size = 0;
            std::size_t read = 0;
            std::size_t remaining = 0;
            bool finished = false;
        };

    private:
        // The data the parser writes to is managed by an exchange
        // This could be a copy since the data inside are pointers, but it can be a reference for now
        exchange &exch;

        // Internal state for parsing a HTTP body, since it can span multiple calls
        body_parsing_status bp_status;

        // Buffer segments for managing compound reads

        util::buffer::buffer_segment request_method_buf;
        util::buffer::buffer_segment request_target_buf;
        util::buffer::buffer_segment request_version_buf;
        util::buffer::buffer_segment response_version_buf;
        util::buffer::buffer_segment response_code_buf;
        util::buffer::buffer_segment response_msg_buf;
        util::buffer::buffer_segment header_buf;
        util::buffer::buffer_segment chunk_header_buf;
        util::buffer::buffer_segment chunk_suffix_buf;

        /*
            Asserts that the mode given is not unknown.
        */
        static void assert_not_unknown(message_mode mode);

        /*
            Gets the reference for where to save the parsed data.
        */
        message &get_data_for_mode(message_mode mode);

        /*
            Checks the expected body size based on the request and response data currently in the parser.
        */
        std::pair<body_size_type, std::size_t> expected_body_size(message_mode mode);

        /*
            Resets the body parsing status.
        */
        void reset_body_parsing_status();

    public:
        http_parser(exchange &exch);

        /*
            Parses the request line from the stream.
        */
        void read_request_line(std::istream &in);

        /*
            Parses the response line from the stream..
        */
        void read_response_line(std::istream &in);

        /*
            Reads the headers from the stream.
        */
        void read_headers(std::istream &in, message_mode mode);

        /*
            Reads the message body from the stream.
            This method is stateful, and it returns the current state after each call.
            State is automatically reset when the body is successfully read.
            Do not switch message mode between reads.
        */
        body_parsing_status read_body(std::istream &in, message_mode mode);
    };
}