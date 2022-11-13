/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <algorithm>
#include <optional>

#include "aether/util/streambuf.hpp"

namespace util::buffer {

namespace double_buffer_detail {

template <typename Buffer>
class double_buffer {
 public:
  constexpr explicit double_buffer(Buffer* input, Buffer* output) : buffers_{input, output} {}

  constexpr explicit double_buffer(Buffer* input)
      : internal_buffer_1_(std::in_place), buffers_{input, &internal_buffer_1_.value()} {}

  constexpr double_buffer()
      : internal_buffer_1_(std::in_place),
        internal_buffer_2_(std::in_place),
        buffers_{&internal_buffer_1_.value(), &internal_buffer_2_.value()} {}

  ~double_buffer() = default;
  double_buffer(const double_buffer& other) = delete;
  double_buffer& operator=(const double_buffer& other) = delete;
  double_buffer(double_buffer&& other) noexcept = delete;
  double_buffer& operator=(double_buffer&& other) noexcept = delete;

  constexpr void swap() { current_ ^= 1; }

  constexpr bool is_swapped() { return current_ == 1; }

  inline void move_and_swap() {
    std::copy(std::istreambuf_iterator<char>(&input()), std::istreambuf_iterator<char>(),
              std::ostreambuf_iterator<char>(&output()));
    swap();
  }

  constexpr Buffer& input() { return *buffers_[current_]; }

  constexpr Buffer& output() { return *buffers_[current_ ^ 1]; }

 private:
  std::optional<Buffer> internal_buffer_1_;
  std::optional<Buffer> internal_buffer_2_;

  Buffer* buffers_[2];
  std::size_t current_ = 0;
};

}  // namespace double_buffer_detail

// Class for managing two buffers, input and output, and the exchange between them.
using double_buffer = double_buffer_detail::double_buffer<streambuf>;

}  // namespace util::buffer
