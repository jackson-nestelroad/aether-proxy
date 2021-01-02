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

        /*
            Sets the URL object the request is targeting.
            Does not update any internal headers.
        */
        void set_target(const url &target);

        /*
            Updates the request host, automatically updating the Host header.
        */
        void update_host(std::string_view host);

        /*
            Updates the request host, automatically updating the Host header.
        */
        void update_host(std::string_view host, port_t port);
        
        /*
            Updates the request host, automatically updating the host header.
        */
        void update_host(const url::network_location &host);

        /*
            Updates the Origin and Referer headers.
        */
        void update_origin_and_referer(std::string_view origin);

        /*
            Updates the Origin and Referer headers.
        */
        void update_origin_and_referer(const url &origin);

        method get_method() const;
        const url &get_target() const;
        std::string get_host_name() const;
        port_t get_host_port() const;

        std::string request_line_string() const;
        std::string absolute_request_line_string() const;
        friend std::ostream &operator<<(std::ostream &out, const request &req);
    };
}