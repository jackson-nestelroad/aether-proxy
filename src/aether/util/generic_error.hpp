/*********************************************

  Copyright (c) Jackson Nestelroad 2023
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace util {

// A generic error type for use with util::result.
//
// Similar to a base exception class.
class generic_error {
 public:
  inline explicit generic_error(std::string message) noexcept : message_(std::move(message)) {}
  ~generic_error() noexcept = default;
  generic_error(const generic_error& other) = default;
  generic_error& operator=(const generic_error& other) = default;
  generic_error(generic_error&& other) noexcept = default;
  generic_error& operator=(generic_error&& other) noexcept = default;

  inline std::string_view message() const noexcept { return message_; }

 protected:
  std::string message_;
};

inline std::ostream& operator<<(std::ostream& out, const generic_error& error) {
  return out << "Error: " << error.message();
}

}  // namespace util
