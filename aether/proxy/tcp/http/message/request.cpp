/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "request.hpp"

namespace proxy::tcp::http {
    request::request() 
        : _method(), target()
    { }

    request::request(method _method, url target, version _version, std::initializer_list<header_pair> headers, const std::string &content)
        : message(_version, headers, content),
        _method(_method),
        target(target)
    { }

    request::request(const request &other) { 
        *this = other;
    }

    request &request::operator=(const request &other) {
        message::operator=(other);
        _method = other._method;
        target = other.target;
        return *this;
    }

    request::request(request &&other) noexcept {
        *this = std::move(other);
    }

    request &request::operator=(request &&other) noexcept {
        message::operator=(std::move(other));
        _method = other._method;
        target = other.target;
        return *this;
    }

    void request::set_method(method _method) {
        this->_method = _method;
    }

    void request::set_target(const url &target) {
        this->target = target;
    }

    void request::update_target(const url &target) {
        this->target = target;
        set_header_to_value("Host", target.netloc.host);
    }

    void request::update_host(std::string_view host) {
        this->target.netloc.host = host;
        this->target.netloc.port = { };
        set_header_to_value("Host", this->target.netloc.host);
    }

    void request::update_host(std::string_view host, port_t port) {
        this->target.netloc.host = host;
        this->target.netloc.port = port;
        set_header_to_value("Host", this->target.netloc.host);
    }

    void request::update_host(const url::network_location &host) {
        this->target.netloc = host;
        set_header_to_value("Host", host.host);
    }

    void request::update_origin_and_referer(std::string_view origin) {
        set_header_to_value("Origin", origin);
        set_header_to_value("Referer", origin);
    }

    void request::update_origin_and_referer(const url &origin) {
        set_header_to_value("Origin", origin.origin_string());
        set_header_to_value("Referer", origin.absolute_string());
    }

    bool request::has_cookies() const {
        return has_header("Cookie");
    }

    cookie_collection request::get_cookies() const {
        return cookie_collection::parse_request_header(get_header("Cookie"));
    }

    void request::set_cookies(const cookie_collection &cookies) {
        set_header_to_value("Cookie", cookies.request_string());
    }

    method request::get_method() const {
        return _method;
    }

    const url &request::get_target() const {
        return target;
    }

    std::string request::get_host_name() const {
        return target.netloc.host;
    }

    port_t request::get_host_port() const {
        return target.get_port(80);
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