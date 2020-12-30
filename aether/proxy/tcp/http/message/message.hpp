/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <map>
#include <string>
#include <vector>
#include <optional>
#include <string_view>
#include <sstream>

#include <aether/proxy/types.hpp>
#include <aether/proxy/error/exceptions.hpp>
#include <aether/util/string.hpp>
#include <aether/proxy/tcp/http/message/version.hpp>

namespace proxy::tcp::http {
    /*
        A base class for a single HTTP message.
        Provides common functionality for request and response classes.
    */
    class message {
    public:
        static constexpr std::string_view CRLF = "\r\n";
        static constexpr std::string_view CRLF_CRLF = "\r\n\r\n";
        static constexpr char SP = ' ';

        using header_pair = std::pair<const std::string, std::string>;
        using headers_map = std::multimap<std::string, std::string, util::string::iless>;

    protected:
        version _version;
        headers_map headers;
        std::string body;

    public:
        message();
        message(version _version, std::initializer_list<header_pair> headers, const std::string &body);
        message(const message &other);
        message &operator=(const message &other);
        message(message &&other) noexcept;
        message &operator=(message &&other) noexcept;

        void set_version(version _version);
        version get_version() const;
        void set_body(const std::string &body);
        std::string get_body() const;
        std::size_t content_length() const;
        const headers_map &all_headers() const;

        void add_header(const std::string &name, const std::string &value);

        /*
            Sets a header to a single value. All previous headers of the same name are removed.
        */
        void set_header_to_value(const std::string &name, const std::string &value);

        /*
            Removes all values for the given header.
        */
        void remove_header(const std::string &name);

        bool has_header(const std::string &name) const;

        /*
            Checks if header has any value except an empty string.
        */
        bool header_is_nonempty(const std::string &name) const;

        /*
            Checks if a header was given the value exactly.
        */
        bool header_has_value(const std::string &name, const std::string &value, bool case_insensitive = false) const;

        /*
            Checks if a header was given the value in a comma-separated list.
        */
        bool header_has_token(const std::string &name, const std::string &value, bool case_insensitive = false) const;

        /*
            Gets the first value for a given header, throwing if it does not exist.
            Since headers can be duplicated, it is safer to use get_all_of_header.
        */
        std::string get_header(const std::string &name) const;

        /*
            Gets the first value for an optional header.
        */
        std::optional<std::string> get_optional_header(const std::string &name) const;

        /*
            Returns a vector of all the values for a given header.
            Will be empty if header does not exist.
        */
        std::vector<std::string> get_all_of_header(const std::string &name) const;

        /*
            Calculates content length and sets the Content-Length header accordingly.
            Overwrites previous values.
        */
        void set_content_length();

        bool should_close_connection() const;

        friend std::ostream &operator<<(std::ostream &out, const message &msg);
    };
}