/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "buffer_segment.hpp"

namespace util::buffer {
    base_segment::base_segment()
        : is_complete(false),
        committed_data(),
        buffer_data(),
        num_bytes_read(0)
    { }

    std::string &base_segment::data_ref() {
        return committed_data;
    }

    std::string base_segment::export_data() const {
        return committed_data;
    }

    bool base_segment::complete() const {
        return is_complete;
    }

    std::size_t base_segment::size() const {
        return committed_data.size();
    }

    std::size_t base_segment::buffer_size() const {
        return buffer_data.size();
    }

    void base_segment::reset() {
        is_complete = false;
        committed_data.clear();
        buffer_data.clear();
        num_bytes_read = 0;
    }
    
    void base_segment::commit_buffer() {
        std::copy(buffer_data.begin(), buffer_data.begin() + num_bytes_read, std::back_inserter(committed_data));
        buffer_data.clear();
        num_bytes_read = 0;
    }

    void base_segment::commit() {
        commit_buffer();
        is_complete = false;
    }

    void base_segment::mark_as_complete() {
        is_complete = true;
    }

    void base_segment::mark_as_incomplete() {
        is_complete = false;
    }

    bool buffer_segment::read_up_to_bytes(std::istream &in, std::size_t bytes) {
        if (!is_complete) {
            buffer_data.resize(bytes);
            is_complete = in.read(&buffer_data[num_bytes_read], bytes - num_bytes_read) && !in.eof();
            num_bytes_read += in.gcount();
            if (is_complete) {
                commit_buffer();
            }
        }
        return is_complete;
    }

    bool buffer_segment::read_until(std::istream &in, char delim) {
        if (!is_complete) {
            std::string temp;
            is_complete = std::getline(in, temp, delim) && !in.eof();
            buffer_data += temp;
            num_bytes_read += temp.length();
            if (is_complete) {
                commit_buffer();
            }
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
                    buffer_data += next + final_delim;
                    num_bytes_read += next.length() + 1;
                }
                // If not successful, not even the last character of the delimiter was found
                if (!success) {
                    return false;
                }
            } while ((num_bytes_read < delim_size) || (buffer_data.substr(num_bytes_read - delim_size, num_bytes_read) != delim));
            // Remove delimiter
            buffer_data = buffer_data.substr(0, num_bytes_read - delim_size);
            num_bytes_read -= delim_size;
            is_complete = !in.fail() && !in.eof();
            if (is_complete) {
                commit_buffer();
            }
        }
        return is_complete;
    }

    bool const_buffer_segment::read_up_to_bytes(proxy::const_streambuf &buf, std::size_t bytes, std::size_t size) {
        if (!is_complete) {
            buffer_data.resize(bytes);
            std::size_t offset = committed_data.size();
            std::size_t bytes_available = size - offset;
            std::size_t to_read = bytes - num_bytes_read;
            if (bytes_available >= to_read) {
                std::copy(boost::asio::buffers_begin(buf) + offset + num_bytes_read, boost::asio::buffers_begin(buf) + offset + to_read, &buffer_data[num_bytes_read]);
                num_bytes_read += to_read;
                is_complete = true;
                commit_buffer();
            }
            else {
                std::copy(boost::asio::buffers_begin(buf) + offset + num_bytes_read, boost::asio::buffers_begin(buf) + size, &buffer_data[num_bytes_read]);
                num_bytes_read += bytes_available;
                is_complete = false;
            }
        }
        return is_complete;
    }
}