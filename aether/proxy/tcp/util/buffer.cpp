/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "buffer.hpp"

namespace proxy::tcp::buffer {
    std::string extract_line(streambuf &buf) {
        std::istream is(&buf);
        std::string line;
        std::getline(is, line);
        return line;
    }

    std::string extract_bytes(streambuf &buf, std::size_t bytes) {
        std::istream is(&buf);
        std::string line(bytes, 0);
        is.read(&line[0], bytes);
        return line;
    }

    bool read_until(std::istream &in, std::string &dest, char delim) {
        if (!std::getline(in, dest, delim) || in.eof()) {
            for (std::size_t i = 0, length = dest.length(); i < length; ++i) {
                in.unget();
            }
            dest.clear();
            return false;
        }
        return true;
    }

    bool read_until(std::istream &in, std::string &dest, std::string_view delim) {
        char final_delim = *delim.rbegin();
        std::size_t delim_size = delim.length();
        std::size_t total_size;
        do {
            std::string next;
            if (!std::getline(in, next, final_delim) || in.eof()) {
                for (std::size_t i = 0, length = next.length(); i < length; ++i) {
                    in.unget();
                }
                dest.clear();
                return false;
            }
            dest += next + final_delim;
            total_size = dest.size();
        } while ((total_size < delim_size) || (dest.substr(total_size - delim_size, delim_size) != delim));
        // Remove delimiter
        dest = dest.substr(0, total_size - delim_size);
        return !in.fail() && !in.eof();
    }
}