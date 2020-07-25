/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

// Macro for quickly generating a sub-exception that sends the exception class name into the reason
#define GENERATE_EXCEPTION(name, base, msg) \
class name : public base { \
public: \
    inline name(const std::string &str = msg) \
        : base(#name ": " + str) \
    { } \
};

#include <string>
#include <stdexcept>

namespace proxy::error {
    /*
        Base exception class for any error that comes out of the proxy.
    */
    class base_exception
        : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    /*
        Base exception class for any error related to HTTP parsing.
    */
    class http_exception
        : public base_exception {
        using base_exception::base_exception;
    };

    /*
        Base exception for any error related to TLS.
    */
    class tls_exception
        : public base_exception {
        using base_exception::base_exception;
    };

    GENERATE_EXCEPTION(invalid_option_exception, base_exception, "Invalid option");
    GENERATE_EXCEPTION(ipv6_exception, base_exception, "IPv6 exception raised");
    GENERATE_EXCEPTION(invalid_operation_exception, base_exception, "Invalid operation");
    GENERATE_EXCEPTION(acceptor_exception, base_exception, "Exception in acceptor class");
    GENERATE_EXCEPTION(parser_exception, base_exception, "Exception in parser class");
    GENERATE_EXCEPTION(server_not_connected_exception, base_exception, "Server is not connected");

    namespace http {
        GENERATE_EXCEPTION(invalid_method_exception, http_exception, "Invalid HTTP method");
        GENERATE_EXCEPTION(invalid_status_exception, http_exception, "Invalid HTTP status");
        GENERATE_EXCEPTION(invalid_version_exception, http_exception, "Invalid HTTP version");
        GENERATE_EXCEPTION(invalid_target_host_exception, http_exception, "Invalid target host");
        GENERATE_EXCEPTION(invalid_target_port_exception, http_exception, "Invalid target port");
        GENERATE_EXCEPTION(invalid_request_line_exception, http_exception, "Invalid HTTP request line");
        GENERATE_EXCEPTION(invalid_header_exception, http_exception, "Invalid HTTP header");
        GENERATE_EXCEPTION(header_not_found_exception, http_exception, "Header was not found");
        GENERATE_EXCEPTION(invalid_body_size_exception, http_exception, "Invalid HTTP body size");
        GENERATE_EXCEPTION(body_size_too_large_exception, http_exception, "Given body size is exceeds limit");
        GENERATE_EXCEPTION(invalid_chunked_body_exception, http_exception, "Malformed chunked-encoding body");
        GENERATE_EXCEPTION(no_response_exception, http_exception, "HTTP exchange has no response");
        GENERATE_EXCEPTION(invalid_response_line_exception, http_exception, "Invalid HTTP response line");
    }

    namespace tls {
        GENERATE_EXCEPTION(invalid_client_hello_exception, tls_exception, "Invalid ClientHello message");
        GENERATE_EXCEPTION(read_access_violation_exception, tls_exception, "Read access violation (not enough data)");
        GENERATE_EXCEPTION(tls_service_exception, tls_exception, "Exception in TLS service");
        GENERATE_EXCEPTION(invalid_ssl_method_exception, tls_exception, "Invalid SSL version");
        GENERATE_EXCEPTION(invalid_cipher_suite_exception, tls_exception, "Invalid cipher suite");
        GENERATE_EXCEPTION(invalid_trusted_certificates_file, tls_exception, "Invalid verify file");
        GENERATE_EXCEPTION(invalid_cipher_suite_list_exception, tls_exception, "Invalid cipher suite list");
        GENERATE_EXCEPTION(invalid_alpn_protos_list_exception, tls_exception, "Invalid ALPN protocol list");
        GENERATE_EXCEPTION(ssl_context_exception, tls_exception, "Failed to create and configure SSL context");
    }
}

#undef GENERATE_EXCEPTION