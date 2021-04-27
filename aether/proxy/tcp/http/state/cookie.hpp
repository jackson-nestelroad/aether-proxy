/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <string>
#include <map>
#include <optional>
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <aether/proxy/tcp/http/message/url.hpp>
#include <aether/util/string.hpp>

namespace proxy::tcp::http {
    /*
        Class representing a single HTTP cookie, which is used to record state
            across multiple HTTP requests.
    */
    class cookie {
    private:
        static const std::locale locale;
        static const boost::posix_time::ptime epoch;

        std::map<std::string, std::string, util::string::iless> attributes;

    public:
        std::string name;
        std::string value;

        cookie(std::string_view name, std::string_view value);

        std::optional<std::string_view> get_attribute(std::string_view attribute) const;
        void set_attribute(std::string_view attribute, std::string_view value);

        std::optional<boost::posix_time::ptime> expires() const;
        std::optional<std::string_view> domain() const;

        void expire();
        void set_expires(const boost::posix_time::ptime &time);
        void set_domain(std::string_view domain);
        
        std::string request_string() const;
        std::string response_string() const;

        bool operator<(const cookie &other) const;

        /*
            Parses a cookie from a "Set-Cookie" header.
            The header string is invalid and should be ignored if a std::nullopt is returned.
        */
        static std::optional<cookie> parse_set_header(std::string_view header);

        friend std::ostream &operator<<(std::ostream &output, const cookie &cookie);
    };
}