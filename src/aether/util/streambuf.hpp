/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <string.h>

#include <algorithm>
#include <boost/asio/buffer.hpp>
#include <memory>
#include <stdexcept>
#include <streambuf>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "aether/util/console.hpp"

namespace util::buffer {

namespace buffer_detail {

// Typed extension of boost::asio buffer class.
//
// Inherits from the boost::asio classes because a few boost::asio methods require these explicit types.
template <typename T, typename AsioBase>
class base_buffer : public AsioBase {
 public:
  using value_type = T;

  base_buffer(T* data, std::size_t size) noexcept : AsioBase(data, size) {}

  T* data() noexcept { return static_cast<T*>(this->AsioBase::data()); }

  const T* data() const noexcept { return static_cast<const T*>(this->AsioBase::data()); }

  T* begin() noexcept { return static_cast<T*>(this->AsioBase::data()); }

  T* end() noexcept { return static_cast<T*>(this->AsioBase::data()) + this->AsioBase::size(); }

  const T* begin() const noexcept { return static_cast<const T*>(this->AsioBase::data()); }

  const T* end() const noexcept { return static_cast<const T*>(this->AsioBase::data()) + this->AsioBase::size(); }

  base_buffer& operator+=(std::size_t n) noexcept {
    this->AsioBase::operator+=(n);
    return *this;
  }

  T& operator[](std::size_t i) noexcept { return (static_cast<T*>(this->AsioBase::data()))[i]; }

  const T& operator[](std::size_t i) const noexcept { return (static_cast<T*>(this->AsioBase::data()))[i]; }
};

}  // namespace buffer_detail

using mutable_buffer = buffer_detail::base_buffer<char, boost::asio::mutable_buffer>;
using const_buffer = buffer_detail::base_buffer<const char, boost::asio::const_buffer>;

using mutable_buffer_sequence = std::vector<mutable_buffer>;
using const_buffer_sequence = std::vector<const_buffer>;

// Custom data management class based on std::streambuf.
//
// Uses logic from boost::asio::streambuf with a few minor tweaks and additions:
// - Move semantics.
// - std::string_view support.
template <typename Allocator = std::allocator<char>>
class basic_streambuf : public std::streambuf {
 public:
  explicit basic_streambuf(std::size_t max_size = std::numeric_limits<std::size_t>::max(),
                           const Allocator& allocator = Allocator())
      : max_size_(max_size), buffer_(allocator) {
    std::size_t pend = std::min<std::size_t>(max_size_, buffer_delta);
    buffer_.resize(std::max<std::size_t>(pend, 1));
    setg(&buffer_[0], &buffer_[0], &buffer_[0]);
    setp(&buffer_[0], &buffer_[0] + pend);
  }

  basic_streambuf(basic_streambuf&& other) noexcept { this->operator=(std::move(other)); }

  basic_streambuf& operator=(basic_streambuf&& other) noexcept {
    if (this != &other) {
      max_size_ = other.max_size_;
      buffer_ = std::move(other.buffer_);

      // Update internal pointers
      swap(other);
    }
    return *this;
  }

  // Moves the buffer to another streambuf object and performs a hard reset on this object, allowing it to be reused
  // later.
  void move_and_reset(basic_streambuf& dest) noexcept {
    if (this != &dest) {
      dest.operator=(std::move(*this));
      make_new();
    }
  }

  // Clears all data from the input sequence.
  void reset() { consume(size()); }

  // Reinitializes this object to the initial state.
  void make_new() { this->operator=(basic_streambuf{}); }

  // Returns the size of the input sequence.
  std::size_t size() const noexcept { return pptr() - gptr(); }

  // Returns the maximum size of the buffer.
  std::size_t max_size() const noexcept { return max_size_; }

  // Returns the current capacity of the buffer.
  std::size_t capacity() const noexcept { return buffer_.capacity(); }

  // Returns a constant view of the input sequence.
  const_buffer data() const noexcept { return {gptr(), size() * sizeof(char_type)}; }

  // Returns a list of buffers that represents the input sequence.
  const_buffer_sequence data_sequence() const noexcept { return {data()}; }

  // Returns a view to an output sequence of a given size.
  mutable_buffer prepare(std::size_t n) noexcept {
    reserve(n);
    return {pptr(), n * sizeof(char_type)};
  }

  // Returns a list of buffers that represents the output sequence of a given size.
  mutable_buffer_sequence prepare_sequence(std::size_t n) { return {prepare(n)}; }

  // Moves characters from the output sequence to the input sequence.
  void commit(std::size_t n) {
    n = std::min<std::size_t>(n, epptr() - pptr());
    pbump(static_cast<int>(n));
    setg(eback(), gptr(), pptr());
  }

  // Removes characters from the input sequence.
  void uncommit(std::size_t n) {
    n = std::min<std::size_t>(n, size());
    pbump(-static_cast<int>(n));
    setg(eback(), gptr(), pptr());
  }

  // Removes characters from the input sequence.
  void consume(std::size_t n) {
    if (egptr() < pptr()) {
      setg(&buffer_[0], gptr(), pptr());
    }
    if (gptr() + n > pptr()) {
      n = size();
    }
    gbump(static_cast<int>(n));
  }

  // Returns a string view to the input sequence.
  constexpr std::string_view string_view() const noexcept { return {gptr(), size() * sizeof(char_type)}; }

  // Returns a mutable view to the available input sequence.
  mutable_buffer mutable_input() noexcept { return {gptr(), size() * sizeof(char_type)}; }

  // Returns a mutable view to the available output sequence.
  mutable_buffer mutable_output() noexcept { return {pptr(), (epptr() - pptr()) * sizeof(char_type)}; }

 protected:
  static constexpr std::size_t buffer_delta = 1 << 7;

  int_type underflow() {
    if (gptr() < pptr()) {
      setg(&buffer_[0], gptr(), pptr());
      return traits_type::to_int_type(*gptr());
    } else {
      return traits_type::eof();
    }
  }

  int_type overflow(int_type c) {
    if (!traits_type::eq_int_type(c, traits_type::eof())) {
      if (pptr() == epptr()) {
        std::size_t buffer_size = size();
        std::size_t delta = max_size_ - buffer_size;
        if (buffer_size < max_size_ && delta < buffer_delta) {
          reserve(delta);
        } else {
          reserve(buffer_delta);
        }
      }

      *pptr() = traits_type::to_char_type(c);
      pbump(1);
      return c;
    }
    return traits_type::not_eof(c);
  }

  void reserve(std::size_t n) {
    // Get current stream positions as offsets.
    std::size_t gnext = gptr() - &buffer_[0];
    std::size_t pnext = pptr() - &buffer_[0];
    std::size_t pend = epptr() - &buffer_[0];

    // Check if there is already enough space in the put area.
    if (n <= pend - pnext) {
      return;
    }

    // Shift get area to the start of buffer.
    if (gnext > 0) {
      pnext -= gnext;
      std::memmove(&buffer_[0], &buffer_[0] + gnext, pnext);
    }

    // Ensure buffer is large enough to hold at least the specified size.
    if (n > pend - pnext) {
      if (n <= max_size_ && pnext <= max_size_ - n) {
        pend = pnext + n;
        buffer_.resize(std::max<std::size_t>(pend, 1));
      } else {
        throw std::length_error{"streambuf too long"};
      }
    }

    // Update stream positions
    setg(&buffer_[0], &buffer_[0], &buffer_[0] + pnext);
    setp(&buffer_[0] + pnext, &buffer_[0] + pend);
  }

 private:
  std::size_t max_size_;
  std::vector<std::streambuf::char_type, Allocator> buffer_;

  basic_streambuf(const basic_streambuf& other) = delete;
  basic_streambuf& operator=(const basic_streambuf& other) = delete;
};

// Proxy object that conforms our streambuf to ASIO's DynamicBuffer_v1 type requirements.
template <typename Allocator>
class asio_dynamic_buffer_v1 {
 public:
  using const_buffers_type = const_buffer_sequence;
  using mutable_buffers_type = mutable_buffer_sequence;

  asio_dynamic_buffer_v1(basic_streambuf<Allocator>& sb) : sb_(sb) {}

  asio_dynamic_buffer_v1(const asio_dynamic_buffer_v1& other) : sb_(other.sb_) {}

  asio_dynamic_buffer_v1(asio_dynamic_buffer_v1&& other) noexcept : sb_(other.sb_) {}

  std::size_t size() const noexcept { return sb_.size(); }

  std::size_t max_size() const noexcept { return sb_.max_size(); }

  std::size_t capacity() const noexcept { return sb_.capacity(); }

  const_buffers_type data() const noexcept { return sb_.data_sequence(); }

  mutable_buffers_type prepare(std::size_t n) { return sb_.prepare_sequence(n); }

  void commit(std::size_t n) { sb_.commit(n); }

  void consume(std::size_t n) { sb_.consume(n); }

 private:
  basic_streambuf<Allocator>& sb_;
};

using streambuf = basic_streambuf<>;

}  // namespace util::buffer
