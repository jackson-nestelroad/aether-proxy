/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <string>
#include <map>
#include <optional>
#include <iostream>

#include <aether/proxy/tcp/http/message/url.hpp>
#include <aether/util/string.hpp>

namespace proxy::tcp::http {
    /*
        Class representing a single HTTP cookie, which is used to record state
            across multiple HTTP requests.
    */
    struct cookie {
        std::string name;
        std::string value;
        std::map<std::string, std::string, util::string::iless> attributes;

        std::optional<std::string> get_attribute(std::string_view attribute) const;
        void set_attribute(std::string_view attribute, std::string_view value);

        std::optional<std::string> domain() const;

        std::string to_string() const;

        /*
            Parses a cookie from a "Set-Cookie" header.
            The header string is invalid and should be ignored if a std::nullopt is returned.
        */
        static std::optional<cookie> parse_set_header(std::string_view header);
    };

    std::ostream &operator<<(std::ostream &output, const cookie &cookie);
}