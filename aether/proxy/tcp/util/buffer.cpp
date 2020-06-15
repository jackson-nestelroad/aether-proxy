/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "buffer.hpp"

namespace proxy::tcp::buffer {
    bool read_bytes(std::istream &in, std::string &dest, std::size_t bytes) {
        dest.resize(bytes);
        if (!in.read(&dest[0], bytes) || in.eof()) {
            for (std::size_t i = 0, length = dest.length(); i < length; ++i) {
                in.unget();
            }
            dest.clear();
            return false;
        }
        return true;
    }

    std::size_t read_up_to_bytes(std::istream &in, std::string &dest, std::size_t bytes) {
        dest.resize(bytes);
        in.read(&dest[0], bytes);
        return in.gcount();
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