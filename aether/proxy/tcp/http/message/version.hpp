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
        Enumeration type for versions of HTTP.
    */
    enum class version {
#define X(name, string) name,
        HTTP_VERSIONS(X)
#undef X
    };

    namespace convert {
        /*
            Converts an HTTP version to string.
        */
        std::string_view to_string(version v);

        /*
            Converts a string to an HTTP version.
        */
        version to_version(std::string_view str);
    }

    std::ostream &operator<<(std::ostream &output, version v);
}