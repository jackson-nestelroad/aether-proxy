/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <boost/lexical_cast.hpp>
#include <boost/asio/ssl.hpp>

#include <aether/util/string.hpp>
#include <aether/proxy/error/exceptions.hpp>

namespace proxy::tcp::tls::openssl {
    // Allows SSL methods
#define SSL_METHODS(X) \
X(boost::asio::ssl::context::sslv23, "SSLv23") \
X(boost::asio::ssl::context::sslv2, "SSLv2") \
X(boost::asio::ssl::context::sslv3, "SSLv3") \
X(boost::asio::ssl::context::tlsv1, "TLSv1") \
X(boost::asio::ssl::context::tlsv11, "TLSv1.1") \
X(boost::asio::ssl::context::tlsv12, "TLSv1.2") \
X(boost::asio::ssl::context::tlsv13, "TLSv1.3")

    using ssl_method = boost::asio::ssl::context_base::method;

    namespace convert {
        constexpr std::string_view to_string(ssl_method method) {
            switch (method) {
#define X(name, str) case name: return str;
                SSL_METHODS(X)
#undef X
            }
            throw error::tls::invalid_ssl_method_exception { };
        }

        ssl_method to_ssl_method(std::string_view str);
    }
}

namespace boost {
    template <>
    proxy::tcp::tls::openssl::ssl_method lexical_cast(const std::string &str);

    namespace asio::ssl {
        std::ostream &operator<<(std::ostream &output, context_base::method m);
    }
}
