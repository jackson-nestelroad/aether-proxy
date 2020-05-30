/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/tcp/http/message/message.hpp>
#include <aether/proxy/tcp/http/message/status.hpp>

namespace proxy::tcp::http {
    /*
        A single HTTP response.
    */
    class response 
        : public message {
    private:
        status status_code;

    public:
        using ptr = std::shared_ptr<response>;

        response();
        response(version _version, status status_code, std::initializer_list<header_pair> headers, const std::string &content);
        response(const response &other);
        response &operator=(const response &other);

        void set_status(status status_code);

        status get_status() const;

        bool is_1xx() const;
        bool is_2xx() const;
        bool is_3xx() const;
        bool is_4xx() const;
        bool is_5xx() const;

        friend std::ostream &operator<<(std::ostream &out, const response &res);
    };
}