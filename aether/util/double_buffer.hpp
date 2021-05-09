/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <optional>

#include <aether/util/streambuf.hpp>

namespace util::buffer {
    namespace _impl {
        template <typename Buffer>
        class double_buffer {
        private:
            std::optional<Buffer> internal_buffer_1;
            std::optional<Buffer> internal_buffer_2;

            Buffer *buffers[2];
            std::size_t current = 0;

        public:
            constexpr explicit double_buffer(Buffer *input, Buffer *output)
                : buffers { input, output }
            { }

            constexpr explicit double_buffer(Buffer *input)
                : internal_buffer_1(std::in_place),
                buffers { input, &internal_buffer_1.value() }
            { }

            constexpr double_buffer()
                : internal_buffer_1(std::in_place),
                internal_buffer_2(std::in_place),
                buffers { &internal_buffer_1.value(), &internal_buffer_2.value() }
            { }

            ~double_buffer() = default;
            double_buffer(const double_buffer &other) = delete;
            double_buffer &operator=(const double_buffer &other) = delete;
            double_buffer(double_buffer &&other) noexcept = delete;
            double_buffer &operator=(double_buffer &&other) noexcept = delete;

            constexpr void swap() {
                current ^= 1;
            }

            constexpr bool is_swapped() {
                return current == 1;
            }

            inline void move_and_swap() {
                std::copy(std::istreambuf_iterator<char>(&input()), std::istreambuf_iterator<char>(), std::ostreambuf_iterator<char>(&output()));
                swap();
            }

            constexpr Buffer &input() {
                return *buffers[current];
            }

            constexpr Buffer &output() {
                return *buffers[current ^ 1];
            }
        };
    }
    /*
        Class for managing two buffers, input and output, and the exchange between them.
    */
    using double_buffer = _impl::double_buffer<streambuf>;
}