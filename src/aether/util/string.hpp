/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "aether/util/generic_error.hpp"
#include "aether/util/result.hpp"

// Helper functions for operations on std::string.

namespace util::string {

namespace string_detail {

template <typename T>
struct is_string_native
    : std::disjunction<std::is_same<char*, std::decay_t<T>>, std::is_same<const char*, std::decay_t<T>>> {};

template <typename T, typename = void>
struct is_string_class : std::false_type {};

template <typename T, typename Traits, typename Alloc>
struct is_string_class<std::basic_string<T, Traits, Alloc>, void> : std::true_type {};

template <typename T, typename = void>
struct is_string_view : std::false_type {};

template <typename T, typename Traits>
struct is_string_view<std::basic_string_view<T, Traits>, void> : std::true_type {};

template <typename T>
struct is_string : std::disjunction<is_string_native<T>, is_string_class<T>, is_string_view<T>> {};

template <typename T>
struct is_string_container : std::disjunction<is_string_class<T>, is_string_view<T>> {};

}  // namespace string_detail

template <typename T>
using is_string = string_detail::is_string<T>;

template <typename T>
constexpr bool is_string_v = is_string<T>::value;

template <typename T>
using is_string_container = string_detail::is_string_container<T>;

template <typename T>
constexpr bool is_string_container_v = is_string_container<T>::value;

// Returns a string representing the substring from the start index (inclusive) to the end of the string.
constexpr std::string_view substring(std::string_view src, std::size_t start_index) { return src.substr(start_index); }

// Returns a string representing the substring from the start index (inclusive) to the end index (exclusive).
constexpr std::string_view substring(std::string_view src, std::size_t start_index, std::size_t end_index) {
  return src.substr(start_index, end_index == std::string::npos ? end_index : end_index - start_index);
}

// Trims the beginning and end of a string using the character delimiters given as whitespace.
std::string_view trim(std::string_view src, std::string_view whitespace = " \t");

// Splits a string along a delimiter.
template <typename S = std::string>
std::enable_if_t<is_string_container_v<S>, std::vector<S>> split(std::string_view src, char delim) {
  std::vector<std::string> tokens;
  std::size_t prev = 0;
  std::size_t pos = 0;
  while ((pos = src.find(delim, prev)) != std::string::npos) {
    tokens.emplace_back(src.substr(0, pos - prev));
    prev = pos + 1;
  }
  // Push everything else to the end of the string.
  tokens.emplace_back(src.substr(prev));
  return tokens;
}

// Splits a string along a delimiter.
template <typename S = std::string>
std::enable_if_t<is_string_container_v<S>, std::vector<S>> split(std::string_view src, std::string_view delim) {
  std::vector<S> tokens;
  std::size_t delim_size = delim.length();
  std::size_t prev = 0;
  std::size_t pos = 0;
  while ((pos = src.find(delim, prev)) != std::string::npos) {
    tokens.emplace_back(src.substr(0, pos - prev));
    prev = pos + delim_size;
  }
  // Push everything else to the end of the string.
  tokens.emplace_back(src.substr(prev));
  return tokens;
}

// Splits a string along a delimiter, removing any linear white space from each entry.
template <typename S = std::string>
std::enable_if_t<is_string_container_v<S>, std::vector<S>> split_trim(std::string_view src, char delim,
                                                                      std::string_view whitespace = " \t") {
  std::vector<S> tokens;
  std::size_t prev = 0;
  std::size_t pos = 0;
  while ((pos = src.find(delim, prev)) != std::string::npos) {
    std::size_t begin = src.find_first_not_of(whitespace, prev);
    // The whole entry is whitespace.
    //
    // Unless delim contains whitespace, begin is guaranteed to be 0 <= begin <= pos.
    if (begin >= pos) {
      prev = begin == std::string::npos ? begin : begin + 1;
    } else {
      // This would fail if pos == 0, because the whole string would be searched.
      //
      // However, the previous if statement assures that begin < pos, so if pos == 0, begin == 0 because it is the first
      // non-whitespace character, so this statement is never reached.
      std::size_t end = src.find_last_not_of(whitespace, pos - 1);
      // prev <= begin <= end < pos.
      tokens.emplace_back(src.substr(begin, end - begin + 1));
      prev = pos + 1;
    }
  }
  // Push everything else to the end of the string, if it's not whitespace.
  std::size_t begin = src.find_first_not_of(whitespace, prev);
  if (begin != std::string::npos) {
    std::size_t end = src.find_last_not_of(whitespace);
    tokens.emplace_back(src.substr(begin, end - begin + 1));
  }
  return tokens;
}

// Joins a range into a single string with a delimiter.
template <typename Range, typename Value = typename Range::value_type>
std::string join(const Range& range, std::string_view delim) {
  std::ostringstream str;
  auto begin = std::begin(range);
  auto end = std::end(range);

  if (begin != end) {
    std::copy(begin, std::prev(end), std::ostream_iterator<Value>(str, delim.data()));
    begin = prev(end);
  }

  if (begin != end) {
    str << *begin;
  }

  return str.str();
}

// Converts an ASCII string to lowercase.
std::string lowercase(std::string_view src);

bool ends_with(std::string_view str, std::string_view suffix);

// Checks if two case-insensitive strings are equal.
bool iequals_fn(std::string_view a, std::string_view b);

// Lexicographically compares two case-insensitive strings.
bool iless_fn(std::string_view a, std::string_view b);

// Lexicographically compares two case-insensitive strings.
struct iless {
  using is_transparent = void;

  inline bool operator()(std::string_view a, std::string_view b) const { return iless_fn(a, b); }
};

// Checks if two case-insensitive strings are equal.
struct iequals {
  using is_transparent = void;

  inline bool operator()(std::string_view a, std::string_view b) const { return iequals_fn(a, b); }
};

// Hash functor for case-insensitive strings.
struct ihash {
 public:
  std::size_t operator()(std::string_view s) const;

 private:
  std::hash<std::string> hasher_;
};

// Parses a hexadecimal value from a string.
result<std::size_t, generic_error> parse_hexadecimal(std::string_view src);

}  // namespace util::string
