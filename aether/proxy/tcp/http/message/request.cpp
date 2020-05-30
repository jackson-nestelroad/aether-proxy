/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "request.hpp"

namespace proxy::tcp::http {
    request::request() { }

    request::request(method _method, url target, version _version, std::initializer_list<header_pair> headers, const std::string &content)
        : message(_version, headers, content),
        _method(_method),
        target(target)
    { }

    request::request(const request &other) 
        : message(other),
        _method(other._method),
        target(other.target)
    { }

    request &request::operator=(const request &other) {
        message::operator=(other);
        _method = other._method;
        target = other.target;
        return *this;
    }

    void request::set_method(method _method) {
        this->_method = _method;
    }

    void request::set_target(url target) {
        this->target = target;
    }

    void request::set_host(const std::string &host) {
        this->target.netloc.host = host;
        this->target.netloc.port = { };
        set_header_to_value("Host", this->target.netloc.to_string());
    }

    void request::set_host(const std::string &host, port_t port) {
        this->target.netloc.host = host;
        this->target.netloc.port = port;
        set_header_to_value("Host", this->target.netloc.to_string());
    }

    void request::set_host(const url::network_location &host) {
        this->target.netloc = host;
        set_header_to_value("Host", host.to_host_string());
    }

    method request::get_method() const {
        return _method;
    }

    url request::get_target() const {
        return target;
    }

    std::string request::get_host_name() const {
        return target.netloc.host;
    }

    port_t request::get_host_port() const {
        return target.netloc.port.value_or(0);
    }

    std::string request::request_line_string() const {
        std::stringstream out;
        out << _method << ' ';
        out << target << ' ';
        out << _version;
        return out.str();
    }

    std::string request::absolute_request_line_string() const {
        std::stringstream out;
        out << _method << ' ';
        out << target.absolute_string() << ' ';
        out << _version;
        return out.str();
    }

    std::ostream &operator<<(std::ostream &out, const request &req) {
        out << req._method << ' ';
        out << req.target << ' ';
        out << req._version;
        out << message::CRLF;
        out << static_cast<const message &>(req);
        return out;
    }
}