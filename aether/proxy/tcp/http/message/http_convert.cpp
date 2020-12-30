/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "method.hpp"
#include "status.hpp"
#include "version.hpp"

// Function implementations for converting HTTP types to string and vice versa

namespace proxy::tcp::http {
    namespace convert {
        std::string_view to_string(method m) {
            switch (m) {
#define X(name) case method::name: return #name;
                HTTP_METHODS(X)
#undef X
            }
            throw error::http::invalid_method_exception { };
        }

        std::string_view status_to_reason(status s) {
            switch (s) {
#define X(num, name, string) case status::name: return string;
                HTTP_STATUS_CODES(X)
#undef X
            }
            throw error::http::invalid_status_exception { };
        }

        std::string_view to_string(version v) {
            switch (v) {
#define X(name, string) case version::name: return #string;
                HTTP_VERSIONS(X)
#undef X
            }
            throw error::http::invalid_version_exception { };
        }
        
        // These maps convert from string to method using the string hashing function

        struct method_map
            : public std::unordered_map<std::string_view, method> {
            method_map() {
#define X(name) this->operator[](#name) = method::name;
                HTTP_METHODS(X)
#undef X
            }
        };

        struct version_map
            : public std::unordered_map<std::string_view, version> {
            version_map() {
#define X(name, string) this->operator[](#string) = version::name;
                HTTP_VERSIONS(X)
#undef X
            }
        };

        method to_method(std::string_view str) {
            static method_map map;
            auto ptr = map.find(str);
            if (ptr == map.end()) {
                throw error::http::invalid_method_exception { };
            }
            return ptr->second;
        }

        version to_version(std::string_view str) {
            static version_map map;
            auto ptr = map.find(str);
            if (ptr == map.end()) {
                throw error::http::invalid_version_exception { };
            }
            return ptr->second;
        }

        status to_status_from_code(std::string_view str) {
            std::size_t code;
            try {
                code = boost::lexical_cast<std::size_t>(str);
            }
            catch (const boost::bad_lexical_cast &) {
                throw error::http::invalid_status_exception { };
            }
            return to_status_from_code(code);
        }

        status to_status_from_code(std::size_t code) {
            // Allow invalid HTTP statuses
            return static_cast<status>(code);
        }
    }

    // Output operators for HTTP types

    std::ostream &operator<<(std::ostream &output, method m) {
        output << convert::to_string(m);
        return output;
    }

    std::ostream &operator<<(std::ostream &output, status s) {
        output << static_cast<std::size_t>(s);
        return output;
    }

    std::ostream &operator<<(std::ostream &output, version v) {
        output << convert::to_string(v);
        return output;
    }
}

// boost::lexical_cast overloads for HTTP methods to string
// Not as efficient as functions in the convert namespace because they need to catch
// thrown exceptions and re-throw them as boost::bad_lexical_cast

namespace boost {
    template <>
    proxy::tcp::http::method lexical_cast(const std::string &str) {
        try {
            return proxy::tcp::http::convert::to_method(str);
        }
        catch (...) {
            throw bad_lexical_cast { };
        }
    }

    template <>
    proxy::tcp::http::status lexical_cast(const std::string &str) {
        try {
            return proxy::tcp::http::convert::to_status_from_code(str);
        }
        catch (...) {
            throw bad_lexical_cast { };
        }
    }

    template <>
    proxy::tcp::http::version lexical_cast(const std::string &str) {
        try {
            return proxy::tcp::http::convert::to_version(str);
        }
        catch (...) {
            throw bad_lexical_cast { };
        }
    }
}