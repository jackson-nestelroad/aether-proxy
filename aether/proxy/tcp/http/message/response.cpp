/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "response.hpp"

namespace proxy::tcp::http {
    response::response() 
        : status_code(status::ok) 
    { }

    response::response(version _version, status status_code, std::initializer_list<header_pair> headers, const std::string &content)
        : message(_version, headers, content),
        status_code(status_code)
    { }

    response::response(const response &other) { 
        *this = other;
    }

    response &response::operator=(const response &other) {
        message::operator=(other);
        status_code = other.status_code;
        return *this;
    }

    response::response(response &&other) noexcept {
        *this = std::move(other);
    }

    response &response::operator=(response &&other) noexcept {
        message::operator=(std::move(other));
        status_code = other.status_code;
        return *this;
    }

    void response::set_status(status status_code) {
        this->status_code = status_code;
    }

    status response::get_status() const {
        return status_code;
    }

    bool response::is_1xx() const {
        return static_cast<std::size_t>(status_code) / 100 == 1;
    }

    bool response::is_2xx() const {
        return static_cast<std::size_t>(status_code) / 100 == 2;
    }

    bool response::is_3xx() const {
        return static_cast<std::size_t>(status_code) / 100 == 3;
    }

    bool response::is_4xx() const {
        return static_cast<std::size_t>(status_code) / 100 == 4;
    }

    bool response::is_5xx() const {
        return static_cast<std::size_t>(status_code) / 100 == 5;
    }

    std::vector<cookie> response::set_cookie_headers() const {
        std::vector<std::string> cookie_headers = get_all_of_header("Set-Cookie");
        std::vector<cookie> cookies;
        for (const auto &header : cookie_headers) {
            auto &&parsed = cookie::parse_set_header(header);
            if (parsed.has_value()) {
                cookies.emplace_back(parsed.value());
            }
        }
        return cookies;
    }

    std::ostream &operator<<(std::ostream &out, const response &res) {
        out << res._version << ' ';
        out << res.status_code << ' ';
        out << convert::status_to_reason(res.status_code);
        out << message::CRLF;
        out << static_cast<const message &>(res);
        return out;
    }
}