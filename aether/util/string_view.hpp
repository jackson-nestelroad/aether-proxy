/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <string_view>

// Compile-time templates for std::string_view concatenation

namespace util::string_view {
    namespace _impl {
        // String concatenation universal case
        template <const std::string_view &, typename, const std::string_view &, typename>
        struct concat;

        // Concatenate two string views
        template <
            const std::string_view &S1,
            std::size_t... I1,
            const std::string_view &S2,
            std::size_t... I2
        >
        struct concat<S1, std::index_sequence<I1...>, S2, std::index_sequence<I2...>>
        {
            static constexpr const char value[] { S1[I1]..., S2[I2]..., 0 };
        };

        // String to lowercase universal case
        template <const std::string_view &, typename>
        struct to_lowercase;

        // Implementation for one character
        constexpr char to_lowercase_func(char c) {
            return c >= 65 && c <= 90 ? c ^ 0x20 : c;
        }

        // Convert an entire string to lowercase (one character at a time)
        template <
            const std::string_view &S,
            std::size_t... I
        >
        struct to_lowercase<S, std::index_sequence<I...>> {
            static constexpr const char value[] { to_lowercase_func(S[I])..., 0 };
        };
    }

    // String joining universal case
    template <const std::string_view &...>
    struct join;

    // String joining base case
    template <>
    struct join<> {
        static constexpr std::string_view value = "";
    };

    // String joining base case
    template <const std::string_view &S1, const std::string_view &S2>
    struct join<S1, S2> {
        static constexpr std::string_view value =
            _impl::concat<
                S1,
                std::make_index_sequence<S1.size()>,
                S2,
                std::make_index_sequence<S2.size()>
            >::value;
    };

    // String joining recursive case
    template <const std::string_view &S, const std::string_view &... Rest>
    struct join<S, Rest...> {
        static constexpr std::string_view value =
            join<S, join<Rest...>::value>::value;
    };

    template <const std::string_view &... Ss>
    static constexpr auto join_v = join<Ss...>::value;

    // Convert a string to lowercase
    template <const std::string_view &S>
    struct to_lowercase
        : _impl::to_lowercase<S, std::make_index_sequence<S.size()>> 
    { };

    template <const std::string_view &S>
    static constexpr auto to_lowercase_v = to_lowercase<S>::value;
}