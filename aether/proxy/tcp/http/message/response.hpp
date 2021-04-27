/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/tcp/http/message/message.hpp>
#include <aether/proxy/tcp/http/message/status.hpp>
#include <aether/proxy/tcp/http/state/cookie_collection.hpp>

namespace proxy::tcp::http {
    /*
        A single HTTP response.
    */
    class response 
        : public message {
    private:
        status status_code;

    public:
        response();
        response(version _version, status status_code, std::initializer_list<header_pair> headers, std::string_view content);
        response(const response &other);
        response &operator=(const response &other);
        response(response &&other) noexcept;
        response &operator=(response &&other) noexcept;

        status get_status() const;
        void set_status(status status_code);

        bool is_1xx() const;
        bool is_2xx() const;
        bool is_3xx() const;
        bool is_4xx() const;
        bool is_5xx() const;

        /*
            Returns if there is one or more Set-Cookie headers.
        */
        bool has_cookies() const;

        /*
            Parses and returns the cookies attached to all Set-Cookie headers.
        */
        cookie_collection get_cookies() const;

        /*
            Adds all of the cookies to their own Set-Cookie header.
            Any previous cookie headers will be deleted.
        */
        void set_cookies(const cookie_collection &cookies);

        friend std::ostream &operator<<(std::ostream &out, const response &res);
    };
}