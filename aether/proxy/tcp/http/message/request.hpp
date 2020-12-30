/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/tcp/http/message/message.hpp>
#include <aether/proxy/tcp/http/message/method.hpp>
#include <aether/proxy/tcp/http/message/url.hpp>

namespace proxy::tcp::http {
    /*
        A single HTTP request.
    */
    class request 
        : public message {
    private:
        method _method;
        url target;

    public:
        request();
        request(method _method, url target, version _version, std::initializer_list<header_pair> headers, const std::string &content);
        request(const request &other);
        request &operator=(const request &other);
        request(request &&other) noexcept;
        request &operator=(request &&other) noexcept;

        void set_method(method _method);
        void set_target(url target);
        void set_host(const std::string &host);
        void set_host(const std::string &host, port_t port);
        void set_host(const url::network_location &host);

        method get_method() const;
        url get_target() const;
        std::string get_host_name() const;
        port_t get_host_port() const;

        std::string request_line_string() const;
        std::string absolute_request_line_string() const;
        friend std::ostream &operator<<(std::ostream &out, const request &req);
    };
}