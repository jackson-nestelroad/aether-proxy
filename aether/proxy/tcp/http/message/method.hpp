/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>
#include <boost/lexical_cast.hpp>

#include <aether/proxy/error/exceptions.hpp>

// <winnt.h> gets in the way on Windows
#ifdef DELETE
#undef DELETE
#endif

#define HTTP_METHODS(X) \
/* HTTP/1.0 */ \
X(GET) \
X(HEAD) \
X(POST) \
/* HTTP/1.1 */ \
X(OPTIONS) \
X(PUT) \
X(DELETE) \
X(TRACE) \
X(CONNECT) \
/* WebDAV */ \
X(COPY) \
X(LOCK) \
X(MKCOL) \
X(MOVE) \
X(PROPFIND) \
X(PROPPATCH) \
X(UNLOCK) \
X(SEARCH) \
X(BIND) \
X(REBIND) \
X(UNBIND) \
X(ACL) \
/* Versioning Extensions to WebDAV */ \
X(REPORT) \
X(MKACTIVITY) \
X(CHECKOUT) \
X(MERGE) \
/* UPnP */ \
X(MSEARCH) \
X(NOTIFY) \
X(SUBSCRIBE) \
X(UNSUBSCRIBE) \
/* RFC 5789 */ \
X(PATCH) \
/* CalDAV */ \
X(MKCALENDAR) \
/* RFC 2068 */ \
X(LINK) \
X(UNLINK) \
/* Custom */ \
X(PURGE)

namespace proxy::tcp::http {
    /*
        Enum for HTTP method.
    */
    enum class method {
#define X(name) name,
        HTTP_METHODS(X)
#undef X
    };

    constexpr std::string_view method_strings[] = {
#define X(name) #name,
        HTTP_METHODS(X)
#undef X
    };

    /*
        Converts HTTP method to string.
    */
    constexpr std::string_view to_string(method m) {
        return method_strings[static_cast<std::size_t>(m)];
    }

    namespace convert {
        /*
            Converts a string to a HTTP method.
        */
        method to_method(std::string_view str);
    }

    std::ostream &operator<<(std::ostream &output, method m);
}