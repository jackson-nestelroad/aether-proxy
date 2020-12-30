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
    /*
        Permanently slices everything up to the delimiter and returns the sliced chunk.
        If no delimiter is found, the string is unchanged.
    */
    std::string slice_before(std::string &src, std::string_view delim, bool keep_delim = false);

    /*
        Permanently slices everything after the delimiter and returns the sliced chunk.
        If no delimiter is found, the string is unchanged.
    */
    std::string slice_after(std::string &src, std::string_view delim, bool keep_delim = false);

    /*
        Trims the beginning and end of a string using the character delimiters given as whitespace.
    */
    std::string trim(const std::string &src, std::string_view whitespace = " \t");

    /*
        Splits a string along a delimiter.
    */
    std::vector<std::string> split(const std::string &src, char delim);

    /*
        Splits a string along a delimiter.
    */
    std::vector<std::string> split(const std::string &src, std::string_view delim);

    /*
        Splits a string along a delimiter, removing any linear white space from each entry.
    */
    std::vector<std::string> split_trim(const std::string &src, char delim, std::string_view whitespace = " \t");

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
    std::string lowercase(const std::string &src);

    /*
        Checks if two case-insensitive strings are equal.
    */
    bool iequals_fn(const std::string &a, const std::string &b);

    /*
        Lexicographically compares two case-insensitive strings.
    */
    struct iless {
        bool operator()(const std::string &a, const std::string &b) const;
    };

    /*
        Checks if two case-insensitive strings are equal.
    */
    struct iequals {
        bool operator()(const std::string &a, const std::string &b) const;
    };

    /*
        Hash functor for case-insensitive strings.
    */
    struct ihash {
    private:
        std::hash<std::string> hasher;

    public:
        std::size_t operator()(const std::string &s) const;
    };

    /*
        Parses a hexadecimal value from a string.
        Throws std::bad_cast if string is not a hexadecimal integer.
    */
    std::size_t parse_hexadecimal(std::string_view src);
}