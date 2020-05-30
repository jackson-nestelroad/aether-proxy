/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

// Helper functions for operations on std::string

namespace proxy::tcp::http::string {
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
    std::vector<std::string> split(const std::string &src, std::string_view delim);

    /*
        Converts an ASCII string to lowercase.
    */
    std::string lowercase(const std::string &src);

    /*
        Compares two case-insensitive strings.
    */
    bool iequals(const std::string &a, const std::string &b);

    /*
        Lexicographically compares two case-insensitive strings.
    */
    struct iless {
        bool operator()(const std::string &a, const std::string &b) const;
    };

    /*
        Parses a hexadecimal value from a string.
        Throws std::bad_cast if string is not a hexadecimal integer.
    */
    std::size_t parse_hexadecimal(std::string_view src);
}