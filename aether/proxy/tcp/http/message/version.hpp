/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>
#include <boost/lexical_cast.hpp>

#include <aether/proxy/error/exceptions.hpp>

#define HTTP_VERSIONS(X) \
X(http1_0, HTTP/1.0) \
X(http1_1, HTTP/1.1) \
X(http2_0, HTTP/2.0)

namespace proxy::tcp::http {
    /*
        Enum for a HTTP version.
    */
    enum class version {
#define X(name, string) name,
        HTTP_VERSIONS(X)
#undef X
    };

    /*
        Converts HTTP version to string.
    */
    constexpr std::string_view version_strings[] = {
#define X(name, string) #string,
        HTTP_VERSIONS(X)
#undef X
    };

    constexpr std::string_view to_string(version v) {
        return version_strings[static_cast<std::size_t>(v)];
    }

    namespace convert {
        /*
            Converts a string to a HTTP version.
        */
        version to_version(std::string_view str);
    }

    std::ostream &operator<<(std::ostream &output, version v);
}