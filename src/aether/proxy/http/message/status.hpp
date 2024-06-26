/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>
#include <string_view>

#include "aether/proxy/error/error.hpp"

#define HTTP_STATUS_CODES(X)                                                 \
  X(100, continue_, "Continue")                                              \
  X(101, switching_protocols, "Switching Protocols")                         \
  X(103, early_hints, "Early Hints")                                         \
  X(200, ok, "OK")                                                           \
  X(201, created, "Created")                                                 \
  X(202, accepted, "Accepted")                                               \
  X(203, non_authoritative_information, "Non-Authoritative Information")     \
  X(204, no_content, "No Content")                                           \
  X(205, reset_content, "Reset Content")                                     \
  X(206, partial_content, "Partial Content")                                 \
  X(300, multiple_choices, "Multiple Choices")                               \
  X(301, moved_permanently, "Moved Permanently")                             \
  X(302, found, "Found")                                                     \
  X(303, see_other, "See Other")                                             \
  X(304, not_modified, "Not Modified")                                       \
  X(305, use_proxy, "Use Proxy")                                             \
  X(307, temporary_redirect, "Temporary Redirect")                           \
  X(308, permanent_redirect, "Permanent Redirect")                           \
  X(400, bad_request, "Bad Request")                                         \
  X(401, unauthorized, "Unauthorized")                                       \
  X(402, payment_required, "Payment Required")                               \
  X(403, forbidden, "Forbidden")                                             \
  X(404, not_found, "Not Found")                                             \
  X(405, method_not_allowed, "Method Not Allowed")                           \
  X(406, not_acceptible, "Not Acceptible")                                   \
  X(407, proxy_authentication_required, "Proxy Authentication Required")     \
  X(408, request_timeout, "Request Timeout")                                 \
  X(409, conflict, "Conflict")                                               \
  X(410, gone, "Gone")                                                       \
  X(411, length_required, "Length Required")                                 \
  X(412, precondition_failed, "Precondition Failed")                         \
  X(413, payload_too_large, "Payload Too Large")                             \
  X(414, uri_too_long, "URI Too Long")                                       \
  X(415, unsupported_media_type, "Unsupported Media Type")                   \
  X(416, range_not_satisfiable, "Range Not Satisfiable")                     \
  X(417, expectation_failed, "Expectation Failed")                           \
  X(418, im_a_teapot, "I'm a teapot")                                        \
  X(422, unprocessable_entity, "Unprocessable Entity")                       \
  X(425, too_early, "Too Early")                                             \
  X(426, upgrade_required, "Upgrade Required")                               \
  X(428, precondition_required, "Precondition Required")                     \
  X(429, too_many_requests, "Too Many Requests")                             \
  X(431, request_header_fields_too_large, "Request Header Fields Too Large") \
  X(451, unavailable_for_legal_reasons, "Unavailable For Legal Reasons")     \
  X(500, internal_server_error, "Internal Server Error")                     \
  X(501, not_implemented, "Not Implemented")                                 \
  X(502, bad_gateway, "Bad Gateway")                                         \
  X(503, service_unavailable, "Service Unavailable")                         \
  X(504, gateway_timeout, "Gateway Timeout")                                 \
  X(505, http_version_not_supported, "HTTP Version Not Supported")           \
  X(506, variant_also_negotiates, "Variant Also Negotiates")                 \
  X(507, insufficient_storage, "Insufficient Storage")                       \
  X(508, loop_detected, "Loop Detected")                                     \
  X(510, not_extended, "Not Extended")                                       \
  X(511, network_authentication_required, "Network Authentication Required")

namespace proxy::http {

// Enumeration type for an HTTP status code.
enum class status {
#define X(num, name, string) name = num,
  HTTP_STATUS_CODES(X)
#undef X
};

// Converts an HTTP status to string.
result<std::string_view> status_to_reason(status s);

// Converts a status code string to an HTTP status.
status string_to_status(std::string_view str);

// Converts a number to an HTTP status.
status code_to_status(std::size_t code);

std::ostream& operator<<(std::ostream& output, status s);

}  // namespace proxy::http
