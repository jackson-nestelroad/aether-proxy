/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "buffer_segment.hpp"

namespace util::buffer {
    buffer_segment::buffer_segment()
        : is_complete(false),
        read_data(),
        num_bytes_read(0)
    { }

    std::string &buffer_segment::data_ref() {
        return read_data;
    }

    std::string buffer_segment::export_data() const {
        return read_data;
    }

    bool buffer_segment::complete() const {
        return is_complete;
    }

    std::size_t buffer_segment::bytes_read() const {
        return num_bytes_read;
    }

    void buffer_segment::reset() {
        is_complete = false;
        read_data.clear();
        num_bytes_read = 0;
    }

    void buffer_segment::mark_as_complete() {
        is_complete = true;
    }

    void buffer_segment::mark_as_incomplete() {
        is_complete = false;
    }

    bool buffer_segment::read_bytes(std::istream &in, std::size_t bytes) {
        if (!is_complete) {
            read_data.resize(bytes);
            bool success = in.read(&read_data[num_bytes_read], bytes) && !in.eof();
            num_bytes_read += in.gcount();
            is_complete = success;
        }
        return is_complete;
    }

    bool buffer_segment::read_up_to_bytes(std::istream &in, std::size_t bytes) {
        if (!is_complete) {
            read_data.resize(bytes);
            bool success = in.read(&read_data[num_bytes_read], bytes - num_bytes_read) && !in.eof();
            num_bytes_read += in.gcount();
            is_complete = success;
        }
        return is_complete;
    }

    bool buffer_segment::read_until(std::istream &in, char delim) {
        if (!is_complete) {
            std::string temp;
            bool success = std::getline(in, temp, delim) && !in.eof();
            read_data += temp;
            num_bytes_read += temp.length();
            is_complete = success;
        }
        return is_complete;
    }

    bool buffer_segment::read_until(std::istream &in, std::string_view delim) {
        if (!is_complete) {
            char final_delim = *delim.rbegin();
            std::size_t delim_size = delim.length();
            do {
                std::string next;
                bool success = std::getline(in, next, final_delim) && !in.eof();
                if (!next.empty()) {
                    read_data += next + final_delim;
                    num_bytes_read += next.length() + 1;
                }
                if (!success) {
                    return false;
                }
            } while ((num_bytes_read < delim_size) || (read_data.substr(num_bytes_read - delim_size, num_bytes_read) != delim));
            // Remove delimiter
            read_data = read_data.substr(0, num_bytes_read - delim_size);
            is_complete = !in.fail() && !in.eof();
        }
        return is_complete;
    }
}