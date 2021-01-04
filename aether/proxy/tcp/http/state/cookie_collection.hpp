/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/tcp/http/state/cookie.hpp>

namespace proxy::tcp::http {
    /*
        Class representing a collection of cookies
    */
    class cookie_collection {
    private:
        std::map<std::string, cookie> cookies;

    public:
        void update(const cookie_collection &new_cookies);
        std::optional<std::reference_wrapper<cookie>> get(std::string_view name);
        std::optional<std::reference_wrapper<const cookie>> get(std::string_view name) const;
        void set(const cookie &cook);
        void remove(const cookie &cook);

        bool empty() const;
        std::size_t size() const;
        std::string request_string() const;

        inline auto begin() {
            return cookies.begin();
        }
        inline auto end() {
            return cookies.end();
        }
        inline auto begin() const {
            return cookies.begin();
        }
        inline auto end() const {
            return cookies.end();
        }

        static cookie_collection parse_request_header(std::string_view header);
    };
}