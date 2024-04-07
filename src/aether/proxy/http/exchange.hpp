/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <optional>
#include <string_view>

#include "aether/proxy/http/message/request.hpp"
#include "aether/proxy/http/message/response.hpp"

namespace proxy::http {

// Owner of one HTTP request/response pair.
class exchange {
 public:
  exchange() = default;
  ~exchange() = default;
  exchange(const exchange& other) = delete;
  exchange& operator=(const exchange& other) = delete;
  exchange(exchange&& other) noexcept = delete;
  exchange& operator=(exchange&& other) noexcept = delete;

  inline http::request& request() { return req_; }
  inline const http::request& request() const { return req_; }
  http::response& response();
  const http::response& response() const;

  // Creates an empty response within the exchange pair.
  inline http::response& make_response() const { return res_.emplace(); }

  // Sets the response in the exchange pair.
  inline void set_response(const http::response& res) { res_ = res; }

  // Checks if the exchange has any response set.
  inline bool has_response() const { return res_.has_value(); }

  // Sets if the HTTP exchange should mask a CONNECT request.
  //
  // This is useful if the proxy knows that the CONNECT will only lead to more HTTP requests.
  //
  // This is extremely dangerous, and it will likely lead to an incorrect service being selected.
  inline void set_mask_connect(bool val) { mask_connect_ = val; }

  // Returns if the exchange should mask a CONNECT request, treating it like a normal HTTP request.
  inline bool mask_connect() const { return mask_connect_; }

 private:
  static constexpr char no_response_error_message[] =
      "No response object in the HTTP exchange. Assure exchange::make_response or exchange::set_response is called "
      "before accessing the response object.";

  http::request req_;
  mutable std::optional<http::response> res_;
  bool mask_connect_ = false;
};

}  // namespace proxy::http
