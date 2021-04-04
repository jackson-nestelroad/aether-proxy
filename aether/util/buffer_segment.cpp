/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "buffer_segment.hpp"

namespace util::buffer {
    base_segment::base_segment()
        : is_complete(false),
        buffer(),
        bytes_in_buffer(),
        num_bytes_read_last(0)
    { }

    bool base_segment::complete() const {
        return is_complete;
    }

    std::size_t base_segment::bytes_committed() const {
        return buffer.size();
    }

    std::size_t base_segment::bytes_not_committed() const {
        return bytes_in_buffer;
    }

    std::size_t base_segment::bytes_last_read() const {
        return num_bytes_read_last;
    }

    void base_segment::reset() {
        is_complete = false;
        buffer.reset();
        bytes_in_buffer = 0;
    }

    void base_segment::commit_buffer(std::size_t bytes) {
        buffer.commit(std::min(bytes, bytes_in_buffer));
        bytes_in_buffer = 0;
    }

    void base_segment::commit() {
        commit_buffer();
        is_complete = true;
    }

    void base_segment::mark_as_complete() {
        is_complete = true;
    }

    void base_segment::mark_as_incomplete() {
        is_complete = false;
    }

    bool buffer_segment::read_up_to_bytes(std::streambuf &in, std::size_t bytes) {
        if (!is_complete) {
            if (bytes == 0) {
                is_complete = true;
            }
            else {
                auto sequence = buffer.prepare(bytes);
                num_bytes_read_last = in.sgetn(&sequence[bytes_in_buffer], bytes - bytes_in_buffer);
                bytes_in_buffer += num_bytes_read_last;
                if (bytes_in_buffer == bytes) {
                    commit();
                }
            }
        }
        return is_complete;
    }

    bool buffer_segment::read_up_to_bytes(std::istream &in, std::size_t bytes) {
        if (!is_complete) {
            if (bytes == 0) {
                is_complete = true;
            }
            else {
                auto sequence = buffer.prepare(bytes);
                is_complete = in.read(&sequence[bytes_in_buffer], bytes - bytes_in_buffer) && !in.eof();
                num_bytes_read_last = in.gcount();
                bytes_in_buffer += num_bytes_read_last;
                if (is_complete) {
                    commit_buffer();
                }
            }
        }
        return is_complete;
    }

    // TODO: Implement these without istream

    bool buffer_segment::read_until(std::streambuf &in, char delim) {
        std::istream stream(&in);
        return read_until(stream, delim);
    }

    bool buffer_segment::read_until(std::streambuf &in, std::string_view delim) {
        std::istream stream(&in);
        return read_until(stream, delim);
    }

    bool buffer_segment::read_until(std::istream &in, char delim) {
        if (!is_complete) {
            std::string temp;
            is_complete = std::getline(in, temp, delim) && !in.eof();
            std::copy(temp.begin(), temp.end(), std::ostreambuf_iterator(&buffer));
            num_bytes_read_last = temp.length();
            bytes_in_buffer = 0;
        }
        return is_complete;
    }

    bool buffer_segment::ends_with_delim(char delim) {
        const_buffer data = buffer.data();
        std::size_t size = data.size();
        return *(data.end() - 1) == delim;
    }

    bool buffer_segment::ends_with_delim(std::string_view delim) {
        const_buffer data = buffer.data();
        std::size_t size = data.size();
        std::size_t delim_size = delim.size();
        const char *buffer_it = data.end() - delim_size;
        for (std::size_t i = 0; i < delim_size; ++i) {
            if (*(buffer_it + i) != delim[i]) {
                return false;
            }
        }
        return true;
    }

    bool buffer_segment::read_until(std::istream &in, std::string_view delim) {
        if (!is_complete) {
            char final_delim = *delim.rbegin();
            std::size_t delim_size = delim.length();
            do {
                std::string next;
                bool success = static_cast<bool>(std::getline(in, next, final_delim));
                bool eof = in.eof();

                // Commit data to buffer
                if (success) {
                    std::copy(next.begin(), next.end(), std::ostreambuf_iterator(&buffer));
                    num_bytes_read_last = next.length();
                    bytes_in_buffer = 0;

                    if (!eof) {
                        buffer.sputc(final_delim);
                        ++num_bytes_read_last;
                    }
                }
                
                if (!success || eof) {
                    return false;
                }
            } while ((buffer.size() < delim_size) || !ends_with_delim(delim));

            // while loop breaks when buffer ends with delimiter
            is_complete = true;

            // Remove delimiter
            buffer.uncommit(delim.length());
        }
        return is_complete;
    }

    void buffer_segment::read_all(std::streambuf &in) {
        if (!is_complete) {
            std::copy(std::istreambuf_iterator<char>(&in), std::istreambuf_iterator<char>(), std::ostreambuf_iterator<char>(&buffer));
            is_complete = true;
            bytes_in_buffer = 0;
        }
    }

    void buffer_segment::read_all(std::istream &in) {
        if (!is_complete) {
            std::copy(std::istream_iterator<char>(in), std::istream_iterator<char>(), std::ostreambuf_iterator<char>(&buffer));
            is_complete = true;
            bytes_in_buffer = 0;
        }
    }

    bool const_buffer_segment::read_up_to_bytes(const_buffer &buf, std::size_t bytes, std::size_t size) {
        if (!is_complete) {
            if (bytes == 0) {
                is_complete = true;
            }
            else {
                std::size_t offset = buffer.size();
                std::size_t bytes_available = size - offset;
                std::size_t to_read = bytes - bytes_in_buffer;
                auto sequence = buffer.prepare(to_read);
                if (bytes_available >= to_read) {
                    std::copy(buf.begin() + offset + bytes_in_buffer, buf.begin() + offset + to_read, &sequence[bytes_in_buffer]);
                    num_bytes_read_last = to_read;
                    bytes_in_buffer += num_bytes_read_last;
                    is_complete = true;
                    commit_buffer();
                }
                else {
                    std::copy(buf.begin() + offset + bytes_in_buffer, buf.end(), &sequence[bytes_in_buffer]);
                    num_bytes_read_last = bytes_available;
                    bytes_in_buffer += num_bytes_read_last;
                    is_complete = false;
                }
            }
        }
        return is_complete;
    }
}