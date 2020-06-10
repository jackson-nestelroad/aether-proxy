/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <map>
#include <string>
#include <vector>
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
        streambuf content;

    public:
        message();
        message(version _version, std::initializer_list<header_pair> headers, const std::string &content);
        message(const message &other);
        message &operator=(const message &other);

        void set_version(version _version);
        version get_version() const;
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
            Gets the first value for a given header.
            Since headers can be duplicated, it is safer to use get_all_of_header.
        */
        std::string get_header(const std::string &name) const;

        /*
            Returns a vector of all the values for a given header.
            Will be empty if header does not exist.
        */
        std::vector<std::string> get_all_of_header(const std::string &name) const;

        /*
            Returns a reference to the content buffer.
        */
        streambuf &get_content_buf();

        /*
            Returns a constant reference to the content buffer.
        */
        const streambuf &get_content_buf_const() const;

        std::size_t content_length() const;
        void clear_content();

        /*
            Returns an output stream that adds to the content buffer.
        */
        std::ostream content_stream();

        /*
            Calculates content length and sets the Content-Length header accordingly.
            Overwrites previous values.
        */
        void set_content_length();

        bool should_close_connection() const;

        friend std::ostream &operator<<(std::ostream &out, const message &msg);
    };
}