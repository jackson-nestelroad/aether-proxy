/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "response.hpp"

namespace proxy::tcp::http {
    response::response() { }

    response::response(version _version, status status_code, std::initializer_list<header_pair> headers, const std::string &content)
        : message(_version, headers, content),
        status_code(status_code)
    { }

    response::response(const response &other)
        : message(other),
        status_code(other.status_code)
    { }

    response &response::operator=(const response &other) {
        message::operator=(other);
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

    std::ostream &operator<<(std::ostream &out, const response &res) {
        out << res._version << ' ';
        out << res.status_code << ' ';
        out << convert::status_to_reason(res.status_code);
        out << message::CRLF;
        out << static_cast<const message &>(res);
        return out;
    }
}