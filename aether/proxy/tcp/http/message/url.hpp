/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <optional>
#include <string_view>
#include <boost/lexical_cast.hpp>

#include <aether/proxy/types.hpp>
#include <aether/proxy/error/exceptions.hpp>
#include <aether/util/string.hpp>
#include <aether/proxy/tcp/http/message/method.hpp>

namespace proxy::tcp::http {
    /*
        A class representing a single target URL.
    */
    struct url {
        // Delimiters for the search part of the URL
        static constexpr std::string_view search_delims = ";?#";
        // Delimiters for the path and search part of the URL
        static constexpr std::string_view search_path_delims = "/;?#";

        /*
            Enumeration type for a URL's target form.
        */
        enum class target_form {
            origin,
            absolute,
            authority,
            asterisk
        };

        /*
            Structure for the netloc chunk of a URL.
        */
        struct network_location {
            std::string username;
            std::string password;
            std::string host;
            std::optional<port_t> port;

            bool empty() const;
            bool has_hostname() const;
            bool has_port() const;
            std::string to_string() const;
            std::string to_host_string() const;
        };

        target_form form;
        std::string scheme;
        network_location netloc;
        std::string path;
        std::string search;

        std::string to_string() const;
        std::string absolute_string() const;
        std::string full_path() const;

        /*
            Makes an authority form URL object based off of a host and port alone.
        */
        static url make_authority_form(const std::string &host, port_t port);

        /*
            Makes an origin form URL object based on the path and search alone.
        */
        static url make_origin_form(const std::string &path, const std::string &search = { });

        /*
            Parses a URL target.
            The HTTP method is required here because it impacts how the URL is treated.
        */
        static url parse_target(const std::string &str, method verb);
        
        /*
            Parses an absolute form URL string.
        */
        static url parse(const std::string &str);

        /*
            Parses an authority form URL string.
        */
        static url parse_authority_form(const std::string &str);

        /*
            Parses an origin form URL string.
        */
        static url parse_origin_form(const std::string &str);

        /*
            Parses the netloc chunk of a URL string.
        */
        static network_location parse_netloc(const std::string &str);

        /*
            Parses a port number from a string.
            Validates if the port number is valid.
        */
        static port_t parse_port(const std::string &str);
    };

    std::ostream &operator<<(std::ostream &output, const url::network_location &netloc);
    std::ostream &operator<<(std::ostream &output, const url &u);
}