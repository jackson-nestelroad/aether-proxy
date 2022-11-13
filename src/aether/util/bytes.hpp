/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <cstdint>
#include <iterator>
#include <string>
#include <type_traits>

// Utility functions for working with single bytes of data.

namespace util::bytes {

using byte_t = std::uint8_t;
using double_byte_t = std::uint16_t;
using byte_array_t = std::vector<byte_t>;

namespace bytes_detail {

template <int N>
struct is_valid_byte_string_size {
  static constexpr bool value = (N <= 8) && (N > 0);
};

template <int N>
constexpr bool is_valid_byte_string_size_v = is_valid_byte_string_size<N>::value;

template <int N, typename B>
struct is_valid_byte_string {
  static constexpr bool value = is_valid_byte_string_size_v<N> && std::is_integral_v<B>;
};

template <int N, typename B>
constexpr bool is_valid_byte_string_v = is_valid_byte_string<N, B>::value;

template <int N, typename = void>
struct minimum_int;

template <int N>
struct minimum_int<N, std::enable_if_t<(N > 0) && (N <= 1)>> {
  using type = std::uint8_t;
};

template <int N>
struct minimum_int<N, std::enable_if_t<(N > 1) && (N <= 2)>> {
  using type = std::uint16_t;
};

template <int N>
struct minimum_int<N, std::enable_if_t<(N > 2) && (N <= 4)>> {
  using type = std::uint32_t;
};

template <int N>
struct minimum_int<N, std::enable_if_t<(N > 4) && (N <= 8)>> {
  using type = std::uint64_t;
};

template <int N, typename = void>
struct next_int_based_on_size;

template <int N>
struct next_int_based_on_size<N, std::enable_if_t<(N > 0) && (N <= 7)>> {
  using type = typename minimum_int<N + 1>::type;
};

template <int N>
using next_int_based_on_size_t = typename next_int_based_on_size<N>::type;

template <typename B, typename = void>
struct next_int;

template <typename B>
struct next_int<B, std::enable_if_t<std::is_integral_v<B>>> {
  using type = next_int_based_on_size_t<sizeof(B)>;
};

}  // namespace bytes_detail

template <int N>
using minimum_int_t = typename bytes_detail::minimum_int<N>::type;

template <typename B>
using next_int_t = typename bytes_detail::next_int<B>::type;

namespace bytes_detail {

template <int N, typename B>
constexpr std::enable_if_t<N <= 8 && std::is_integral_v<B>, std::uint64_t> concat(B b1) {
  return b1;
}

template <int N, typename B1, typename B2, typename... Bs>
constexpr std::enable_if_t<N <= 8 && std::is_integral_v<B1> && std::is_integral_v<B2>, std::uint64_t> concat(B1 b1,
                                                                                                             B2 b2,
                                                                                                             Bs... bs) {
  auto left = (static_cast<next_int_based_on_size_t<N>>(static_cast<std::make_unsigned_t<B1>>(b1)) << 8) |
              static_cast<next_int_based_on_size_t<N>>(static_cast<std::make_unsigned_t<B2>>(b2));
  return concat<N + 1>(left, bs...);
}

}  // namespace bytes_detail

// Concatenates 1 to 8 bytes into a single byte string.
template <typename B, typename... Bs>
inline std::enable_if_t<std::is_integral_v<B>, std::uint64_t> concat(B b1, Bs... bs) {
  return bytes_detail::concat<1>(b1, bs...);
}

// Inserts N bytes into the given byte array.
// The bytes are derived from the byte string given, with the most-significant byte being inserted first.
template <int N, typename B>
inline std::enable_if_t<(N <= 8) && (N > 0) && std::is_integral_v<B>, void> insert(byte_array_t& dest, B byte_string) {
  for (int i = N - 1; i >= 0; --i) {
    dest.push_back((byte_string >> (8 * i)) & 0xFF);
  }
}

// Inserts N bytes into the given iterator.
// The bytes are derived from the byte string given, with the most-significant byte being inserted first.
template <int N, typename B, typename Iterator,
          typename IteratorCategory = typename std::iterator_traits<Iterator>::iterator_category>
inline std::enable_if_t<(N <= 8) && (N > 0) && std::is_integral_v<B>, void> insert(Iterator dest, B byte_string) {
  for (int i = N - 1; i >= 0; --i, ++dest) {
    *dest = static_cast<byte_t>((byte_string >> (8 * i)) & 0xFF);
  }
}

// Converts a range of values to a single vector in OpenSSL's wire format.
// N is the number of bytes to use for the length prefix.
template <int N, typename Range, typename Value = typename Range::value_type>
inline std::enable_if_t<(N <= 8) && (N > 0) && !std::is_same_v<Value, std::string>, byte_array_t> to_wire_format(
    const Range& range) {
  byte_array_t out;
  std::string str;
  for (const Value& val : range) {
    // Must convert to string.
    str = boost::lexical_cast<std::string>(val);
    insert<N>(out, str.size());
    std::copy(str.begin(), str.end(), std::back_inserter(out));
  }
  return out;
}

// Converts a range of values to a single vector in OpenSSL's wire format.
// N is the number of bytes to use for the length prefix.
// Special implementation for std::string.
template <int N, typename Range, typename Value = typename Range::value_type>
inline std::enable_if_t<(N <= 8) && (N > 0) && std::is_same_v<Value, std::string>, byte_array_t> to_wire_format(
    const Range& range) {
  byte_array_t out;
  for (const std::string& val : range) {
    // No need to convert to string.
    insert<N>(out, val.size());
    std::copy(val.begin(), val.end(), std::back_inserter(out));
  }
  return out;
}

// Converts a range of bytes to a single byte string using network byte order, or big-endian ordering.
// Takes N elements from the range.
template <int N, typename Range, typename Value = typename Range::value_type>
inline std::enable_if_t<(N <= 8) && (N > 0) && std::is_integral_v<Value> && sizeof(Value) == 1, std::uint64_t>
parse_network_byte_order(const Range& range, std::size_t offset = 0) {
  std::uint64_t result = 0;
  auto it = range.begin() + offset;
  const auto& end = range.end();
  for (int i = 1; i <= N && it != end; ++i, ++it) {
    result |= static_cast<std::make_unsigned_t<Value>>(*it) << ((N - i) << 3);
  }
  return result;
}

// Extracts the Nth byte from the end of the given byte string.
template <int N, typename B>
constexpr std::enable_if_t<(N <= 8) && (N > 0) && std::is_integral_v<B> && sizeof(B) >= N, byte_t> extract_byte(
    B word) {
  return static_cast<byte_t>((word >> ((N - 1) << 3)) & 0xFF);
}

}  // namespace util::bytes
