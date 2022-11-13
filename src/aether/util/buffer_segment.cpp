/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "buffer_segment.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string_view>

#include "aether/util/streambuf.hpp"

namespace util::buffer {
base_segment::base_segment() : is_complete_(false), buffer_(), bytes_in_buffer_(), num_bytes_read_last_(0) {}

bool base_segment::complete() const { return is_complete_; }

std::size_t base_segment::bytes_committed() const { return buffer_.size(); }

std::size_t base_segment::bytes_not_committed() const { return bytes_in_buffer_; }

std::size_t base_segment::bytes_last_read() const { return num_bytes_read_last_; }

void base_segment::reset() {
  is_complete_ = false;
  buffer_.reset();
  bytes_in_buffer_ = 0;
}

void base_segment::commit_buffer(std::size_t bytes) {
  buffer_.commit(std::min(bytes, bytes_in_buffer_));
  bytes_in_buffer_ = 0;
}

void base_segment::commit() {
  commit_buffer();
  is_complete_ = true;
}

void base_segment::mark_as_complete() { is_complete_ = true; }

void base_segment::mark_as_incomplete() { is_complete_ = false; }

bool buffer_segment::read_up_to_bytes(std::streambuf& in, std::size_t bytes) {
  if (!is_complete_) {
    if (bytes == 0) {
      is_complete_ = true;
    } else {
      auto sequence = buffer_.prepare(bytes);
      num_bytes_read_last_ = in.sgetn(&sequence[bytes_in_buffer_], bytes - bytes_in_buffer_);
      bytes_in_buffer_ += num_bytes_read_last_;
      if (bytes_in_buffer_ == bytes) {
        commit();
      }
    }
  }
  return is_complete_;
}

bool buffer_segment::read_up_to_bytes(std::istream& in, std::size_t bytes) {
  if (!is_complete_) {
    if (bytes == 0) {
      is_complete_ = true;
    } else {
      auto sequence = buffer_.prepare(bytes);
      is_complete_ = in.read(&sequence[bytes_in_buffer_], bytes - bytes_in_buffer_) && !in.eof();
      num_bytes_read_last_ = in.gcount();
      bytes_in_buffer_ += num_bytes_read_last_;
      if (is_complete_) {
        commit_buffer();
      }
    }
  }
  return is_complete_;
}

// TODO: Implement these without istream.

bool buffer_segment::read_until(std::streambuf& in, char delim) {
  std::istream stream(&in);
  return read_until(stream, delim);
}

bool buffer_segment::read_until(std::streambuf& in, std::string_view delim) {
  std::istream stream(&in);
  return read_until(stream, delim);
}

bool buffer_segment::read_until(std::istream& in, char delim) {
  if (!is_complete_) {
    std::string temp;
    is_complete_ = std::getline(in, temp, delim) && !in.eof();
    std::copy(temp.begin(), temp.end(), std::ostreambuf_iterator(&buffer_));
    num_bytes_read_last_ = temp.length();
    bytes_in_buffer_ = 0;
  }
  return is_complete_;
}

bool buffer_segment::ends_with_delim(char delim) {
  const_buffer data = buffer_.data();
  return *(data.end() - 1) == delim;
}

bool buffer_segment::ends_with_delim(std::string_view delim) {
  const_buffer data = buffer_.data();
  std::size_t delim_size = delim.size();
  const char* buffer_it = data.end() - delim_size;
  for (std::size_t i = 0; i < delim_size; ++i) {
    if (*(buffer_it + i) != delim[i]) {
      return false;
    }
  }
  return true;
}

bool buffer_segment::read_until(std::istream& in, std::string_view delim) {
  if (!is_complete_) {
    char final_delim = *delim.rbegin();
    std::size_t delim_size = delim.length();
    do {
      std::string next;
      bool success = static_cast<bool>(std::getline(in, next, final_delim));
      bool eof = in.eof();

      // Commit data to buffer.
      if (success) {
        std::copy(next.begin(), next.end(), std::ostreambuf_iterator(&buffer_));
        num_bytes_read_last_ = next.length();
        bytes_in_buffer_ = 0;

        if (!eof) {
          buffer_.sputc(final_delim);
          ++num_bytes_read_last_;
        }
      }

      if (!success || eof) {
        return false;
      }
    } while ((buffer_.size() < delim_size) || !ends_with_delim(delim));

    // while loop breaks when buffer ends with delimiter.
    is_complete_ = true;

    // Remove delimiter.
    buffer_.uncommit(delim.length());
  }
  return is_complete_;
}

void buffer_segment::read_all(std::streambuf& in) {
  if (!is_complete_) {
    std::copy(std::istreambuf_iterator<char>(&in), std::istreambuf_iterator<char>(),
              std::ostreambuf_iterator<char>(&buffer_));
    is_complete_ = true;
    bytes_in_buffer_ = 0;
  }
}

void buffer_segment::read_all(std::istream& in) {
  if (!is_complete_) {
    std::copy(std::istream_iterator<char>(in), std::istream_iterator<char>(), std::ostreambuf_iterator<char>(&buffer_));
    is_complete_ = true;
    bytes_in_buffer_ = 0;
  }
}

bool const_buffer_segment::read_up_to_bytes(const_buffer& buf, std::size_t bytes, std::size_t size) {
  if (!is_complete_) {
    if (bytes == 0) {
      is_complete_ = true;
    } else {
      std::size_t offset = buffer_.size();
      std::size_t bytes_available = size - offset;
      std::size_t to_read = bytes - bytes_in_buffer_;
      auto sequence = buffer_.prepare(to_read);
      if (bytes_available >= to_read) {
        std::copy(buf.begin() + offset + bytes_in_buffer_, buf.begin() + offset + to_read, &sequence[bytes_in_buffer_]);
        num_bytes_read_last_ = to_read;
        bytes_in_buffer_ += num_bytes_read_last_;
        is_complete_ = true;
        commit_buffer();
      } else {
        std::copy(buf.begin() + offset + bytes_in_buffer_, buf.end(), &sequence[bytes_in_buffer_]);
        num_bytes_read_last_ = bytes_available;
        bytes_in_buffer_ += num_bytes_read_last_;
        is_complete_ = false;
      }
    }
  }
  return is_complete_;
}

}  // namespace util::buffer
