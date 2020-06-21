/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <cstdint>
#include <type_traits>

// Utility functions for working with single bytes of data

namespace util::bytes {
    using byte = std::uint8_t;
    using double_byte = std::uint16_t;
    using byte_array = std::vector<byte>;

    namespace _impl {
        template <int N, typename B>
        std::enable_if_t<N <= 8 && std::is_integral_v<B>, std::uint64_t> 
        concat(B b1) {
            return b1;
        }

        template <int N, typename B1, typename B2, typename... Bs>
        std::enable_if_t<N <= 8 && std::is_integral_v<B1> && std::is_integral_v<B2>, std::uint64_t> 
        concat(B1 b1, B2 b2, Bs... bs) {
            std::uint64_t left = (b1 << 8) | b2;
            return concat<N + 1>(left, bs...);
        }
    }

    /*
        Concatenas 1 to 8 bytes into a single byte string.
    */
    template <typename B, typename... Bs>
    std::enable_if_t<std::is_integral_v<B>, std::uint64_t> 
    concat(B b1, Bs... bs) {
        return _impl::concat<1>(b1, bs...);
    }

    /*
        Inserts N bytes into the given byte array.
        The bytes are derived from the byte string given, with
            the most-significant byte being inserted first.
    */
    template <int N, typename B>
    std::enable_if_t<(N <= 8) && (N > 0) && std::is_integral_v<B>, void>
    insert(byte_array &dest, B byte_string) {
        for (int i = N - 1; i >= 0; --i) {
            dest.push_back((byte_string >> (8 * i)) & 0xFF);
        }
    }
}