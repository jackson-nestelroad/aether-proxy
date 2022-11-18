/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace util {

// Compile-time type to give string literals as non-type template parameters.
template <std::size_t N>
struct string_literal {
  char value[N]{};

  consteval string_literal(const char (&str)[N]) noexcept { std::copy_n(str, N, value); }

  consteval operator std::string_view() const noexcept { return std::string_view(value, N); }
  consteval auto size() const noexcept { return N; }
  auto operator<=>(const string_literal&) const = default;
  bool operator==(const string_literal&) const = default;
};

}  // namespace util
