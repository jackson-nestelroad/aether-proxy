/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "ssl_method.hpp"

namespace proxy::tcp::tls::openssl {
    namespace convert {
        struct ssl_method_map
            : public std::unordered_map<std::string_view, ssl_method, util::string::ihash, util::string::iequals> {
            ssl_method_map() {
#define X(name, str) this->operator[](str) = name;
                SSL_METHODS(X)
#undef X
            }
        };

        ssl_method to_ssl_method(std::string_view str) {
            static ssl_method_map map;
            auto ptr = map.find(str);
            if (ptr == map.end()) {
                throw error::tls::invalid_ssl_method_exception { };
            }
            return ptr->second;
        }
    }
}

namespace boost {
    template <>
    proxy::tcp::tls::openssl::ssl_method lexical_cast(const std::string &str) {
        try {
            return proxy::tcp::tls::openssl::convert::to_ssl_method(str);
        }
        catch (...) {
            throw bad_lexical_cast { };
        }
    }

    namespace asio::ssl {
        std::ostream &operator<<(std::ostream &output, context_base::method m) {
            return output << ::proxy::tcp::tls::openssl::convert::to_string(m);
        }
    }
}