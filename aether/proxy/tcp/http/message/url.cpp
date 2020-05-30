/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "url.hpp"

namespace proxy::tcp::http {
    url url::make_authority_form(const std::string &host, port_t port) {
        return { target_form::authority, { }, { { }, { }, host, port } };
    }

    url url::make_origin_form(const std::string &path, const std::string &search) {
        return { target_form::origin, { }, { }, path, search };
    }

    url url::parse_authority_form(std::string str) {
        // Host and port are both required
        std::string host = string::slice_before(str, ":");
        port_t port = parse_port(str);
        return make_authority_form(host, port);
    }

    url url::parse_origin_form(std::string str) {
        std::size_t earliest_delim = str.find_first_of(search_delims);
        // Split path and search
        if (earliest_delim != std::string::npos) {
            return make_origin_form(str.substr(0, earliest_delim), str.substr(earliest_delim));
        }
        // Whole string is the path
        else {
            return make_origin_form(str, { });
        }
    }

    // RFC 1808
    // <scheme>://<netloc>/<path>;<params>?<query>#<fragment>
    // We need host and port
    url url::parse(std::string str) {
        std::string scheme = string::slice_before(str, ":");
        // Scheme is actually beginning of netloc, no scheme found
        if (scheme.substr(0, 2) == "//") {
            str = scheme + str;
            scheme.clear();
        }
        
        // Has netloc
        if (str.substr(0, 2) == "//") {
            // Find everything after netloc
            std::size_t earliest_nonslash_delim = str.find_first_of(search_path_delims, 2);
            std::size_t first_slash = str.find('/', 2);
            std::size_t earliest_delim = std::min(earliest_nonslash_delim, first_slash);

            // Parse netloc
            std::string netloc_str = str.substr(0, earliest_delim);
            if (earliest_delim != std::string::npos) {
                str = str.substr(earliest_delim);
            }
            network_location netloc = parse_netloc(netloc_str);

            // Split path and search if applicable
            std::string path;
            if (earliest_nonslash_delim != std::string::npos) {
                if (earliest_nonslash_delim > earliest_delim) {
                    std::size_t offset = earliest_nonslash_delim - earliest_delim;
                    path = str.substr(0, offset);
                    str = str.substr(offset);
                }
            }
            return { target_form::absolute, scheme, netloc, path, str };
        }
        // No netloc, but there is a path
        else if (str[0] == '/') {
            std::size_t earliest_delim = std::string::npos;
            for (char delim : search_delims) {
                std::size_t loc = str.find(delim);
                if (loc < earliest_delim) {
                    earliest_delim = loc;
                }
            }
            std::string path = str.substr(1, earliest_delim);
            str = str.substr(earliest_delim);
            return { target_form::absolute, scheme, { }, path, str };
        }
        // No netloc, no path, just search
        else {
            return { target_form::absolute, scheme, { }, { }, str };
        }
    }

    // Netloc ==> RFC 1738
    // //<user>:<password>@<host>:<port>/<url-path>
    // We are parsing without /<url-path>
    url::network_location url::parse_netloc(std::string str) {
        std::string username, password, port_str;

        static_cast<void>(string::slice_before(str, "//"));

        // Get username and password if applicable
        std::string login_info = string::slice_before(str, "@");
        if (!login_info.empty()) {
            // Password is optional
            password = string::slice_after(login_info, ":");
            username = login_info;
        }
        // Port is optional
        port_str = string::slice_after(str, ":");
        if (port_str.empty()) {
            return { username, password, str, { } };
        }
        else {
            port_t port = parse_port(port_str);
            return { username, password, str, port };
        }
    }

    // Parse port from string, validating its numerical value in the process
    port_t url::parse_port(const std::string &str) {
        std::size_t port_long;
        try {
            port_long = boost::lexical_cast<std::size_t>(str);
            if (port_long > std::numeric_limits<port_t>::max()) {
                throw error::http::invalid_target_port_exception { "Target port out of range" };
            }
            return static_cast<port_t>(port_long);
        }
        catch (const boost::bad_lexical_cast &) {
            throw error::http::invalid_target_port_exception { "Target port invalid" };
        }
    }

    // RFC-7230 Section 5.3
    url url::parse_target(const std::string &str, method verb) {
        if (str == "*") {
            return { target_form::asterisk };
        }
        else if (str[0] == '/') {
            return parse_origin_form(str);
        }
        else if (verb == method::CONNECT) {
            return parse_authority_form(str);
        }
        else {
            return parse(str);
        }
    }

    bool url::network_location::empty() const {
        return username.empty() && password.empty() && host.empty();
    }

    bool url::network_location::has_hostname() const {
        return !host.empty();
    }

    bool url::network_location::has_port() const {
        return port.has_value();
    }

    std::string url::network_location::to_string() const {
        std::stringstream str;
        str << *this;
        return str.str();
    }

    std::string url::network_location::to_host_string() const {
        std::stringstream str;
        str << host;
        if (port.has_value()) {
            str << ':' << port.value();
        }
        return str.str();
    }

    std::string url::to_string() const {
        std::stringstream str;
        str << *this;
        return str.str();
    }

    std::string url::absolute_string() const {
        std::stringstream str;
        if (!scheme.empty()) {
            str << scheme << ':';
            if (!netloc.empty()) {
                str << "//";
            }
        }
        str << netloc;
        if (!path.empty()) {
            str << path;
        }
        str << search;
        return str.str();
    }

    std::string url::full_path() const {
        return path + search;
    }

    std::ostream &operator<<(std::ostream &output, const url &u) {
        if (u.form == url::target_form::asterisk) {
            output << '*';
        }
        else {
            if (u.form != url::target_form::origin) {
                if (!u.scheme.empty()) {
                    output << u.scheme << ':';
                    if (!u.netloc.empty()) {
                        output << "//";
                    }
                }
                output << u.netloc;
            }
            if (!u.path.empty()) {
                output << u.path;
            }
            output << u.search;
        }
        return output;
    }

    std::ostream &operator<<(std::ostream &output, const url::network_location &netloc) {
        if (!netloc.empty()) {
            if (!netloc.username.empty()) {
                output << netloc.username;
                if (!netloc.password.empty()) {
                    output << ':' << netloc.password;
                }
                output << '@';
            }
            output << netloc.to_host_string();
        }
        return output;
    }
}