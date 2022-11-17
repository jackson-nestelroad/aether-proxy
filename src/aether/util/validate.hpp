/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/lexical_cast.hpp>

#include "aether/util/any_invocable.hpp"

// Types and helper functions for input validation.

namespace util::validate {

// Function type for a validation function used to validate command-line options.
template <typename T>
using validate_func_t = any_invocable<bool(const T&)>;

// Base case for resolve_default_value.
// Returns the only value left, regardless of whether it passes the validation function or not.
template <typename T>
T resolve_default_value(validate_func_t<T>, const T& val) {
  return val;
}

// Recursive case for resolve_default_value.
// Returns the first value to pass the validation function.
// Will return the very last value regardless of whether it passes the validation function or not.
template <typename T, typename... Ts>
T resolve_default_value(validate_func_t<T> validate, const T& val, const T& val2, const Ts&... vals) {
  return validate(val) ? val : resolve_default_value<T>(std::move(validate), val2, vals...);
}

// Generates validation functions using comparator classes and a value to compare to.
// Example:
//  `generate<std::greater>(0)` is equivalent to:
//  ```bool (int a) { return std::greater{ }(a, 0); } = bool (int a) { return a > 0; }```
template <template <typename> class Comparator, typename T>
validate_func_t<T> generate(const T& b) {
  return [comp = Comparator<T>(), b](const T& a) { return comp(a, b); };
}

// Returns true if the first type can be cast to the second type using boost::lexical_cast.
// boost::lexical_cast specializations must be defined for compilation.
template <typename Input, typename Result>
bool lexical_castable(const Input& val) {
  try {
    boost::lexical_cast<Result>(val);
    return true;
  } catch (const boost::bad_lexical_cast&) {
    return false;
  }
}

}  // namespace util::validate
