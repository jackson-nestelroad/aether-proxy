/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <functional>

// Helper functions for operations on std::string

namespace util::string {
    inline std::string as_string(std::string_view view) {
        return { view.data(), view.size() };
    }

    /*
        Returns a string representing the substring from the start index (inclusive) to the end of the string.
    */
    constexpr std::string_view substring(std::string_view src, std::size_t start_index) {
        return src.substr(start_index);
    }

    /*
        Returns a string representing the substring from the start index (inclusive) to the end index (exclusive).
    */
    constexpr std::string_view substring(std::string_view src, std::size_t start_index, std::size_t end_index) {
        return src.substr(start_index, end_index == std::string::npos ? end_index : end_index - start_index);
    }

    /*
        Trims the beginning and end of a string using the character delimiters given as whitespace.
    */
    std::string_view trim(std::string_view src, std::string_view whitespace = " \t");

    /*
        Splits a string along a delimiter.
    */
    std::vector<std::string> split(std::string_view src, char delim);

    /*
        Splits a string along a delimiter.
    */
    std::vector<std::string> split(std::string_view src, std::string_view delim);

    /*
        Splits a string along a delimiter, removing any linear white space from each entry.
    */
    std::vector<std::string> split_trim(std::string_view src, char delim, std::string_view whitespace = " \t");

    /*
        Joins a range into a single string with a delimiter.
    */
    template <typename Range, typename Value = typename Range::value_type>
    std::string join(const Range &range, std::string_view delim) {
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

    /*
        Converts an ASCII string to lowercase.
    */
    std::string lowercase(std::string_view src);

    bool ends_with(std::string_view str, std::string_view suffix);

    /*
        Checks if two case-insensitive strings are equal.
    */
    bool iequals_fn(std::string_view a, std::string_view b);

    /*
        Lexicographically compares two case-insensitive strings.
    */
    struct iless {
        bool operator()(std::string_view a, std::string_view b) const;
    };

    /*
        Checks if two case-insensitive strings are equal.
    */
    struct iequals {
        bool operator()(std::string_view a, std::string_view b) const;
    };

    /*
        Hash functor for case-insensitive strings.
    */
    struct ihash {
    private:
        std::hash<std::string> hasher;

    public:
        std::size_t operator()(std::string_view s) const;
    };

    /*
        Parses a hexadecimal value from a string.
        Throws std::bad_cast if string is not a hexadecimal integer.
    */
    std::size_t parse_hexadecimal(std::string_view src);
}