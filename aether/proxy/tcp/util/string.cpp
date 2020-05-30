/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "string.hpp"

namespace proxy::tcp::http::string {
    std::string slice_before(std::string &src, std::string_view delim, bool keep_delim) {
        std::size_t pos = src.find(delim);
        std::size_t delim_size = delim.length();
        if (pos == std::string::npos) {
            return { };
        }
        std::string res = src.substr(0, pos);
        src = src.substr(keep_delim ? pos : pos + delim_size);
        return res;
    }

    std::string slice_after(std::string &src, std::string_view delim, bool keep_delim) {
        std::size_t pos = src.find(delim);
        std::size_t delim_size = delim.length();
        if (pos == std::string::npos) {
            return { };
        }
        std::string res = src.substr(pos + delim_size);
        src = src.substr(0, keep_delim ? pos + delim_size : pos);
        return res;
    }

    std::string trim(const std::string &src, std::string_view whitespace) {
        std::size_t begin = src.find_first_not_of(whitespace);
        if (begin == std::string::npos) {
            return { };
        }
        std::size_t end = src.find_last_not_of(whitespace);
        return src.substr(begin, end - begin + 1);
    }

    std::vector<std::string> split(const std::string &src, std::string_view delim) {
        std::vector<std::string> tokens;
        std::size_t delim_size = delim.length();
        std::size_t prev = 0;
        std::size_t pos = 0;
        while ((pos = src.find(delim, prev)) != std::string::npos) {
            tokens.push_back(src.substr(0, pos - prev));
            prev = pos + delim_size;
        }
        // Push everything else to the end of the string
        tokens.push_back(src.substr(prev));
        return tokens;
    }

    std::string lowercase(const std::string &src) {
        std::string out;
        std::transform(src.begin(), src.end(), std::back_inserter(out),
            [](char c) { return std::tolower(c); });
        return out;
    }

    bool iequals(const std::string &a, const std::string &b) {
        return std::equal(a.begin(), a.end(), b.begin(), b.end(),
            [](char a, char b) {
                return std::tolower(a) == std::tolower(b);
            });
    }

    bool iless::operator()(const std::string &a, const std::string &b) const {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
            [](char a, char b) {
                return std::tolower(a) < std::tolower(b);
            });
    }

    std::size_t parse_hexadecimal(std::string_view src) {
        std::size_t out;
        std::stringstream ss;
        ss << std::hex << src;
        if (!(ss >> out) || !(ss >> std::ws).eof()) {
            throw std::bad_cast { };
        }
        return out;
    }
}