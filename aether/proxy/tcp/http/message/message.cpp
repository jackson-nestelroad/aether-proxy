/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "message.hpp"

namespace proxy::tcp::http {
    message::message() { }

    message::message(version _version, std::initializer_list<header_pair> headers, const std::string &content)
        : _version(_version),
        headers(headers)
    { 
        std::ostream strm(&this->content);
        strm << content;
    }

    message::message(const message &other) 
        : _version(other._version),
        headers(other.headers)
    {
        std::size_t bytes_copied = boost::asio::buffer_copy(content.prepare(other.content.size()), other.content.data());
        content.commit(bytes_copied);
    }

    message &message::operator=(const message &other) {
        std::size_t bytes_copied = boost::asio::buffer_copy(content.prepare(other.content.size()), other.content.data());
        content.commit(bytes_copied);
        headers = other.headers;
        _version = other._version;
        return *this;
    }

    void message::set_version(version _version) {
        this->_version = _version;
    }

    version message::get_version() const {
        return _version;
    }

    const message::headers_map &message::all_headers() const {
        return headers;
    }

    void message::add_header(const std::string &name, const std::string &value) {
        headers.insert({ name, value });
    }

    void message::set_header_to_value(const std::string &name, const std::string &value) {
        remove_header(name);
        add_header(name, value);
    }

    void message::remove_header(const std::string &name) {
        headers.erase(name);
    }

    bool message::has_header(const std::string &name) const {
        return headers.find(name) != headers.end();
    }

    bool message::header_is_nonempty(const std::string &name) const {
        auto equal_range = headers.equal_range(name);
        return std::all_of(equal_range.first, equal_range.second,
            [](auto pair) { return !pair.second.empty(); });
    }

    bool message::header_has_value(const std::string &name, const std::string &value, bool case_insensitive) const {
        auto equal_range = headers.equal_range(name);
        if (case_insensitive) {
            return std::any_of(equal_range.first, equal_range.second,
                [&value](auto pair) { return util::string::iequals_fn(pair.second, value); });
        }
        else {
            return std::any_of(equal_range.first, equal_range.second,
                [&value](auto pair) { return pair.second == value; });
        }
    }
    
    bool message::header_has_token(const std::string &name, const std::string &value, bool case_insensitive) const {
        auto equal_range = headers.equal_range(name);
        if (case_insensitive) {
            return std::any_of(equal_range.first, equal_range.second,
                [&value](auto pair) {
                    std::vector<std::string> tokens = util::string::split(pair.second, ",");
                    return std::any_of(tokens.begin(), tokens.end(),
                        [&value](auto str) {
                            return util::string::iequals_fn(str, value);
                        });
                });
        }
        else {
            return std::any_of(equal_range.first, equal_range.second,
                [&value](auto pair) {
                    std::vector<std::string> tokens = util::string::split(pair.second, ",");
                    return std::any_of(tokens.begin(), tokens.end(),
                        [&value](auto str) {
                            return str == value;
                        });
                });
        }
    }

    std::string message::get_header(const std::string &name) const {
        auto it = headers.find(name);
        if (it == headers.end()) {
            throw error::http::header_not_found_exception { "Header \"" + name + "\" does not exist" };
        }
        return it->second;
    }

    std::vector<std::string> message::get_all_of_header(const std::string &name) const {
        auto equal_range = headers.equal_range(name);
        std::vector<std::string> out;
        std::transform(equal_range.first, equal_range.second, std::back_inserter(out),
            [](auto it) { return it.second; });
        return out;
    }

    streambuf &message::get_content_buf() {
        return content;
    }

    const streambuf &message::get_content_buf_const() const {
        return content;
    }

    std::size_t message::content_length() const {
        return content.size();
    }

    void message::clear_content() {
        content.consume(content.size());
    }

    std::ostream message::content_stream() {
        return std::ostream(&content);
    }

    void message::set_content_length() {
        add_header("Content-Length", boost::lexical_cast<std::string>(content_length()));
    }


    bool message::should_close_connection() const {
        if (has_header("Connection")) {
            auto connection_header = get_header("Connection");
            if (connection_header == "keep-alive") {
                return false;
            }
            if (connection_header == "close") {
                return true;
            }
        }
        return _version == version::http1_0;
    }

    std::ostream &operator<<(std::ostream &out, const message &msg) {
        for (const auto &[name, value] : msg.headers) {
            out << name << ": " << value << message::CRLF;
        }
        out << message::CRLF;

        std::size_t content_length = msg.content.size();
        if (msg.header_has_token("Transfer-Encoding", "chunked")) {
            out << std::hex << content_length << message::CRLF;
            out.write(static_cast<const char *>(msg.content.data().begin()->data()), content_length);
            out << message::CRLF;
            out << '0' << message::CRLF_CRLF;
        }
        else {
            out.write(static_cast<const char *>(msg.content.data().begin()->data()), content_length);
        }
        return out;
    }
}