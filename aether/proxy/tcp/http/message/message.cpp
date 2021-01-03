/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "message.hpp"

namespace proxy::tcp::http {
    message::message()
        : _version(version::http1_1)
    { }

    message::message(version _version, std::initializer_list<header_pair> headers, const std::string &body)
        : _version(_version),
        headers(headers),
        body(body)
    { }

    message::message(const message &other) {
        *this = other;
    }

    message &message::operator=(const message &other) {
        headers = other.headers;
        _version = other._version;
        body = other.body;
        return *this;
    }

    message::message(message &&other) noexcept {
        *this = std::move(other);
    }

    message &message::operator=(message &&other) noexcept {
        headers = other.headers;
        _version = other._version;
        body = std::move(other.body);
        return *this;
    }

    void message::set_version(version _version) {
        this->_version = _version;
    }

    version message::get_version() const {
        return _version;
    }

    void message::set_body(std::string_view body) {
        this->body = body;
    }

    std::string message::get_body() const {
        return body;
    }

    std::size_t message::content_length() const {
        return body.length();
    }

    const message::headers_map &message::all_headers() const {
        return headers;
    }

    void message::add_header(std::string_view name, std::string_view value) {
        headers.insert({ name.data(), value.data() });
    }

    void message::set_header_to_value(const std::string &name, std::string_view value) {
        remove_header(name);
        add_header(name, value);
    }

    void message::remove_header(const std::string &name) {
        headers.erase(name.data());
    }

    bool message::has_header(const std::string &name) const {
        return headers.find(name) != headers.end();
    }

    bool message::header_is_nonempty(const std::string &name) const {
        auto equal_range = headers.equal_range(name);
        return std::all_of(equal_range.first, equal_range.second,
            [](auto pair) { return !pair.second.empty(); });
    }

    bool message::header_has_value(const std::string &name, std::string_view value, bool case_insensitive) const {
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
    
    bool message::header_has_token(const std::string &name, std::string_view value, bool case_insensitive) const {
        auto equal_range = headers.equal_range(name);
        if (case_insensitive) {
            return std::any_of(equal_range.first, equal_range.second,
                [&value](const auto &pair) {
                    std::vector<std::string> tokens = util::string::split_trim(pair.second, ',');
                    return std::any_of(tokens.begin(), tokens.end(),
                        [&value](const auto &str) {
                            return util::string::iequals_fn(str, value);
                        });
                });
        }
        else {
            return std::any_of(equal_range.first, equal_range.second,
                [&value](const auto &pair) {
                    std::vector<std::string> tokens = util::string::split_trim(pair.second, ',');
                    return std::any_of(tokens.begin(), tokens.end(),
                        [&value](const auto &str) {
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

    std::optional<std::string> message::get_optional_header(const std::string &name) const {
        auto it = headers.find(name);
        return it == headers.end() ? std::optional<std::string> { } : it->second;
    }

    std::vector<std::string> message::get_all_of_header(const std::string &name) const {
        auto equal_range = headers.equal_range(name);
        std::vector<std::string> out;
        std::transform(equal_range.first, equal_range.second, std::back_inserter(out),
            [](auto it) { return it.second; });
        return out;
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

        if (msg.header_has_token("Transfer-Encoding", "chunked")) {
            out << std::hex << msg.body.length() << message::CRLF;
            out << msg.body;
            out << message::CRLF;
            out << '0' << message::CRLF_CRLF;
        }
        else {
            out << msg.body;
        }
        return out;
    }
}