/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "http_parser.hpp"

namespace proxy::tcp::http::http1 {
    http_parser::http_parser(exchange &exch) 
        : exch(exch)
    { }

    void http_parser::assert_not_unknown(message_mode mode) {
        if (mode == message_mode::unknown) {
            throw error::parser_exception { "Cannot parse data for unknown mode" };
        }
    }

    message &http_parser::get_data_for_mode(message_mode mode) {
        assert_not_unknown(mode);
        return mode == message_mode::request ? static_cast<message &>(exch.get_request()) : static_cast<message &>(exch.get_response());
    }

    void http_parser::read_request_line(std::istream &in) {
        if (!request_method_buf.read_until(in, message::SP)
            || !request_target_buf.read_until(in, message::SP)
            || !request_version_buf.read_until(in, message::CRLF)) {
            throw error::http::invalid_request_line_exception { "Could not read request line" };
        }

        // Exceptions will propogate
        request &req = exch.get_request();
        method verb = convert::to_method(request_method_buf.export_data());
        req.set_method(verb);
        req.set_version(convert::to_version(request_version_buf.export_data()));
        url target = url::parse_target(request_target_buf.export_data(), verb);
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
        response &res = exch.get_response();
        res.set_version(convert::to_version(response_version_buf.export_data()));
        res.set_status(convert::to_status_from_code(response_code_buf.export_data()));
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

            auto next_line = header_buf.export_data();
            header_buf.reset();

            if (next_line.empty()) {
                break;
            }
            std::size_t delim = next_line.find(':');
            if (delim == std::string::npos) {
                throw error::http::invalid_header_exception { "No value set for header \"" + next_line + "\"" };
            }
            std::string name = next_line.substr(0, delim);
            std::string value = next_line.substr(delim + 1);
            value = util::string::trim(value);
            msg.add_header(name, value);

            header_buf.reset();
        }
    }

    std::pair<http_parser::body_size_type, std::size_t> http_parser::expected_body_size(message_mode mode) {
        static constexpr std::pair<http_parser::body_size_type, std::size_t> none = { body_size_type::none, 0 };
        bool for_request = mode == message_mode::request;
        request &req = exch.get_request();
        if (for_request) {
            if (req.has_header("Expect") && req.get_header("Expect") == "100-continue") {
                return none;
            }
        }
        else {
            response &res = exch.get_response();
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

    http_parser::body_parsing_status http_parser::read_body(std::istream &in, message_mode mode) {
        message &msg = get_data_for_mode(mode);
        // Initial read, set up state
        if (bp_status.mode == message_mode::unknown) {
            auto pair = expected_body_size(mode);
            // No body to read at all, already successful
            if (pair.first == body_size_type::none) {
                return { mode, body_size_type::none, 0, 0, 0, true };
            }

            std::size_t body_size_limit = program::options::instance().body_size_limit;
            if (pair.second > body_size_limit) {
                throw error::http::body_size_too_large_exception { };
            }
            else if (pair.first == body_size_type::all) {
                pair.second = body_size_limit;
            }
            // We are good to go
            bp_status = { mode, pair.first, pair.second, 0, pair.second };
        }

        // Use buffer::read_until because it will place read characters back into the stream
        // if it fails to find the delimiter.
        // This assures we can return out, read more data from the socket into the stream,
        // and resume reading with the correct amount of information.

        streambuf &content = msg.get_content_buf();
        if (bp_status.type == body_size_type::chunked) {
            while (true) {
                std::size_t bytes_to_read;
                // Need to read chunk header
                if (bp_status.expected_size == 0) {
                    // Could not read chunk header
                    // Return out to let the socket read again
                    if (!chunk_header_buf.read_until(in, message::CRLF)) {
                        break;
                    }

                    std::string line = chunk_header_buf.export_data();
                    chunk_header_buf.reset();

                    try {
                        bytes_to_read = bp_status.expected_size = util::string::parse_hexadecimal(line);
                    }
                    catch (const std::bad_cast &) {
                        throw error::http::invalid_chunked_body_exception { };
                    }
                }
                // We are in the middle of a chunk
                else {
                    bytes_to_read = bp_status.remaining;
                }
                // Going to exceed the limit
                std::size_t body_size_limit = program::options::instance().body_size_limit;
                if (bp_status.read + bytes_to_read > body_size_limit) {
                    throw error::http::body_size_too_large_exception { };
                }
                in.read(static_cast<char *>(content.prepare(bytes_to_read).begin()->data()), bytes_to_read);
                auto just_read = in.gcount();
                bp_status.read += just_read;
                content.commit(just_read);

                // Didn't read the correct amount of data
                // We likely need to read more from the socket
                // If there is no more to read, the service will cancel the request
                if (just_read != bytes_to_read) {
                    if (bp_status.remaining == 0) {
                        bp_status.remaining = bp_status.expected_size - just_read;
                    }
                    else {
                        bp_status.remaining -= just_read;
                    }
                    break;
                }
                // Read the correct amount of data
                else {
                    // Remove suffix, which is a trailing CRLF
                    if (!chunk_suffix_buf.read_until(in, message::CRLF)) {
                        // Could not find suffix
                        // Set remaining to 0 and return to read from socket and come back here
                        bp_status.remaining = 0;
                        break;
                    }

                    std::string line = chunk_suffix_buf.export_data();
                    chunk_suffix_buf.reset();

                    // Found an invalid suffix
                    if (!line.empty()) {
                        throw error::http::invalid_chunked_body_exception { };
                    }
                    // Everything was successful, reset fields
                    else {
                        bp_status.remaining = 0;
                        // This was the last chunk
                        if (bp_status.expected_size == 0) {
                            bp_status.finished = true;
                            break;
                        }
                        // Reset to 0 to read another chunk
                        else {
                            bp_status.expected_size = 0;
                        }
                    }
                }
            }
        }
        // Given or all, both use remaining and read fields
        else {
            // Read into content buffer
            in.read(static_cast<char *>(content.prepare(bp_status.remaining).begin()->data()), bp_status.remaining);
            auto just_read = in.gcount();
            bp_status.read += just_read;
            content.commit(just_read);
            
            std::size_t body_size_limit = program::options::instance().body_size_limit;
            // Input stream could be empty at this point, so simply return out and read from the socket again
            // Unless we are waiting for this as our end condition
            if (just_read == 0) {
                if (bp_status.expected_size == 0 || bp_status.type == body_size_type::all) {
                    bp_status.remaining = 0;
                    bp_status.finished = true;
                }
            }
            // Read over the limit
            else if (bp_status.read > body_size_limit) {
                throw error::http::body_size_too_large_exception { };
            }
            // Ran out of characters to read, but there may still be more
            else if (in.eof() && in.fail()) {
                bp_status.remaining -= just_read;
            }
            // Finished
            else {
                bp_status.remaining = 0;
                bp_status.finished = true;
            }
        }

        // Reset data when finished
        if (bp_status.finished) {
            auto out = bp_status;
            reset_body_parsing_status();
            return out;
        }
        return bp_status;
    }
}