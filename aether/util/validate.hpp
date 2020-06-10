/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <functional>

// Types and helper functions for input validation

namespace util::validate {
    /*
        Base case for resolve_default_value.
        Returns the only value left, regardless of whether it passes the validation function or not.
    */
    template <typename T>
    T resolve_default_value(const std::function<bool(const T &)> &validate, const T &val) {
        return val;
    }

    /*
        Recursive case for resolve_default_value.
        Returns the first value to pass the validation function.
        Will return the very last value regardless of whether it passes the validation function or not.
    */
    template <typename T, typename... Ts>
    T resolve_default_value(const std::function<bool(const T &)> &validate, const T &val, const T &val2, const Ts &... vals) {
        return validate(val) ? val : resolve_default_value<T>(validate, val2, vals...);
    }

    // Function type for a validation function used to validate command-line options.
    template <typename T>
    using validate_func = std::function<bool(const T &)>;

    /*
        Generates validation functions using comparator classes and a value to compare to.
        Example:
            generate<std::greater>(0)
                = bool (int a) { return std::greater{ }(a, 0); }
                = bool (int a) { return a > 0; }
    */
    template <template <typename> class Comparator, typename T>
    validate_func<T> generate(const T &b) {
        return [comp = Comparator<T>(), b](const T &a) {
            return comp(a, b);
        };
    }
}