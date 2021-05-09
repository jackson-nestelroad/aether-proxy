/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "http_parser.hpp"
#include <aether/proxy/server_components.hpp>

namespace proxy::tcp::http::http1 {
    http_parser::http_parser(exchange &exch, server_components &components) 
        : exch(exch),
        options(components.options)
    { }

    void http_parser::assert_not_unknown(message_mode mode) {
        if (mode == message_mode::unknown) {
            throw error::parser_error_exception { "Cannot parse data for unknown mode" };
        }
    }

    message &http_parser::get_data_for_mode(message_mode mode) {
        assert_not_unknown(mode);
        return mode == message_mode::request ? static_cast<message &>(exch.request()) : static_cast<message &>(exch.response());
    }

    void http_parser::read_request_line(std::istream &in) {
        if (!request_method_buf.read_until(in, message::SP)
            || !request_target_buf.read_until(in, message::SP)
            || !request_version_buf.read_until(in, message::CRLF)) {
            throw error::http::invalid_request_line_exception { "Could not read request line" };
        }

        // Exceptions will propogate
        request &req = exch.request();
        method verb = convert::to_method(request_method_buf.string_view());
        req.set_method(verb);
        req.set_version(convert::to_version(request_version_buf.string_view()));
        url target = url::parse_target(request_target_buf.string_view(), verb);
        req.set_target(target);

        request_method_buf.reset();
        request_target_buf.reset();
        request_version_buf.reset();
    }

    void http_parser::read_response_line(std::istream &in) {
        if (!response_version_buf.read_until(in, message::SP)
            || !response_code_buf.read_until(in, message::SP)
            || !response_msg_buf.read_until(in, message::CRLF)) {
            throw error::http::invalid_response_line_exception { "Could not read response line" };
        }

        // Exceptions will propogate
        response &res = exch.response();
        res.set_version(convert::to_version(response_version_buf.string_view()));
        res.set_status(convert::to_status_from_code(response_code_buf.string_view()));
        // Message is discarded, we generate it ourselves when we need it

        response_version_buf.reset();
        response_code_buf.reset();
        response_msg_buf.reset();
    }
    
    void http_parser::read_headers(std::istream &in, message_mode mode) {
        message &msg = get_data_for_mode(mode);
        while (true) {
            if (!header_buf.read_until(in, message::CRLF)) {
                throw error::http::invalid_header_exception { "Error when reading header" };
            }

            auto next_line = header_buf.string_view();

            if (next_line.empty()) {
                header_buf.reset();
                break;
            }
            std::size_t delim = next_line.find(':');
            if (delim == std::string::npos) {
                throw error::http::invalid_header_exception { out::string::stream("No value set for header \"", next_line, "\"") };
            }
            std::string_view name = next_line.substr(0, delim);
            std::string_view value = next_line.substr(delim + 1);
            value = util::string::trim(value);
            msg.add_header(name, value);

            header_buf.reset();
        }
    }

    std::pair<http_parser::body_size_type, std::size_t> http_parser::expected_body_size(message_mode mode) {
        static constexpr std::pair<http_parser::body_size_type, std::size_t> none = { body_size_type::none, 0 };
        bool for_request = mode == message_mode::request;
        request &req = exch.request();
        if (for_request) {
            if (req.has_header("Expect") && req.get_header("Expect") == "100-continue") {
                return none;
            }
        }
        else {
            response &res = exch.response();
            if (req.get_method() == method::HEAD) {
                return none;
            }
            if (res.is_1xx()) {
                return none;
            }
            if (res.get_status() == status::no_content) {
                return none;
            }
            if (res.get_status() == status::not_modified) {
                return none;
            }
            if (res.get_status() == status::ok && req.get_method() == method::CONNECT) {
                return none;
            }
        }
        message &msg = get_data_for_mode(mode);

        if (msg.header_has_token("Transfer-Encoding", "chunked")) {
            return { body_size_type::chunked, 0 };
        }

        if (msg.has_header("Content-Length")) {
            auto sizes = msg.get_all_of_header("Content-Length");
            bool different_sizes = std::adjacent_find(sizes.begin(), sizes.end(), std::not_equal_to<>()) != sizes.end();
            if (different_sizes) {
                throw error::http::invalid_body_size_exception { "Conflicting Content-Length headers" };
            }
            try {
                long long size = boost::lexical_cast<long long>(sizes[0]);
                if (size < 0) {
                    throw boost::bad_lexical_cast { };
                }
                return { body_size_type::given, static_cast<std::size_t>(size) };
            }
            catch (const boost::bad_lexical_cast &) {
                throw error::http::invalid_body_size_exception { "Invalid Content-Length value" };
            }
        }

        // Default cases
        if (for_request) {
            return none;
        }
        return { body_size_type::all, 0 };
    }

    void http_parser::reset_body_parsing_status() {
        // Use default values
        bp_status = { };
    }

    bool http_parser::read_body(std::istream &in, message_mode mode) {
        // Initial read, set up state
        if (bp_status.mode == message_mode::unknown) {
            auto pair = expected_body_size(mode);
            // No body to read at all, already successful
            if (pair.first == body_size_type::none) {
                return true;
            }

            std::size_t body_size_limit = options.body_size_limit;
            if (pair.second > body_size_limit) {
                throw error::http::body_size_too_large_exception { };
            }
            else if (pair.first == body_size_type::all) {
                pair.second = body_size_limit;
            }
            // We are good to go
            bp_status = { mode, pair.first, pair.second, 0 };
        }

        // Use buffer::read_until because it will place read characters back into the stream
        // if it fails to find the delimiter.
        // This assures we can return out, read more data from the socket into the stream,
        // and resume reading with the correct amount of information.

        if (bp_status.type == body_size_type::chunked) {
            while (true) {
                // Need to read chunk header
                if (!bp_status.next_chunk_size_known) {
                    // Could not read chunk header
                    // Return out to let the socket read again
                    if (!chunk_header_buf.read_until(in, message::CRLF)) {
                        break;
                    }

                    std::string_view line = chunk_header_buf.string_view();

                    try {
                        bp_status.expected_size = util::string::parse_hexadecimal(line);
                    }
                    catch (const std::bad_cast &) {
                        throw error::http::invalid_chunked_body_exception { };
                    }

                    chunk_header_buf.reset();

                    // Going to exceed the limit
                    std::size_t body_size_limit = options.body_size_limit;
                    if (bp_status.read + bp_status.expected_size > body_size_limit) {
                        throw error::http::body_size_too_large_exception { };
                    }

                    bp_status.next_chunk_size_known = true;
                }

                // Need more data from the socket
                // If there is no more to read, the service will cancel the request
                if (!body_buf.read_up_to_bytes(in, bp_status.expected_size)) {
                    break;
                }
                // Read all of this chunk
                else {
                    // Remove suffix, which is a trailing CRLF
                    if (!chunk_suffix_buf.read_until(in, message::CRLF)) {
                        // Could not find suffix, return out to hopefully get more data from the socket
                        break;
                    }

                    std::string_view line = chunk_suffix_buf.string_view();

                    // Found an invalid suffix
                    if (!line.empty()) {
                        throw error::http::invalid_chunked_body_exception { };
                    }
                    // Everything was successful, reset fields
                    else {
                        chunk_suffix_buf.reset();
                        // This was the last chunk
                        if (bp_status.expected_size == 0) {
                            bp_status.finished = true;
                            break;
                        }
                        // Reset to 0 to read another chunk
                        else {
                            bp_status.read += bp_status.expected_size;
                            bp_status.next_chunk_size_known = false;
                            bp_status.expected_size = 0;
                            // Keep the chunk in the body buffer, but allow the next chunk to be read
                            body_buf.mark_as_incomplete();
                        }
                    }
                }
            }
        }
        else if (bp_status.type == body_size_type::given) {
            // Read all of body into buffer
            if (body_buf.read_up_to_bytes(in, bp_status.expected_size)) {
                bp_status.read = bp_status.expected_size;
                bp_status.finished = true;
            }
        }
        // Read until EOF, which is rare but allowed by the standard
        else {
            body_buf.read_all(in);
            body_buf.mark_as_incomplete();
            
            auto just_read = body_buf.bytes_last_read();
            bp_status.read += just_read;
            if (bp_status.read > options.body_size_limit) {
                throw error::http::body_size_too_large_exception { };
            }

            if (just_read == 0) {
                bp_status.finished = true;
            }
        }

        // Reset data when finished
        if (bp_status.finished) {
            // TODO: Perhaps avoid this copy from buffer => message.body
            get_data_for_mode(mode).set_body(body_buf.string_view());
            body_buf.reset();
            reset_body_parsing_status();
            return true;
        }

        return false;
    }
}