/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <type_traits>
#include <boost/asio.hpp>

#include <aether/util/bytes.hpp>

// Various definitions used across the codebase

namespace proxy {
    namespace _meta {
        // Mask implemenation universal case
        template <bool B, typename T, template <typename...> class SmartPtr>
        struct _mask_impl { };

        // Mask implementation true case
        template <typename T, template <typename...> class SmartPtr>
        struct _mask_impl<true, T, SmartPtr> {
            using type = T;
            using ptr = SmartPtr<T>;
        };

        /*
            Tests if a given class template is a smart pointer.
        */
        template <template <typename...> typename T>
        struct is_smart_ptr : std::false_type { };
        template <>
        struct is_smart_ptr<std::unique_ptr> : std::true_type { };
        template <>
        struct is_smart_ptr<std::shared_ptr> : std::true_type { };
        template <>
        struct is_smart_ptr<std::weak_ptr> : std::true_type { };

        /*
            Masks an existing type in a struct that re-exposes its type and a smart pointer.
            Used for shortening the names of boost::asio:: types in a common format.
        */
        template <typename T, template <typename...> class SmartPtr>
        struct _mask : _mask_impl<is_smart_ptr<SmartPtr>::value, T, SmartPtr> { };
    }

    using port_t = unsigned short;
    using milliseconds = boost::posix_time::milliseconds;
    using streambuf = boost::asio::streambuf;
    using const_streambuf = boost::asio::streambuf::const_buffers_type;

    using byte = util::bytes::byte;
    using double_byte = util::bytes::double_byte;
    using byte_array = util::bytes::byte_array;

    // Various types of callback functions
    using callback = std::function<void()>;
    using err_callback = std::function<void(const boost::system::error_code &)>;
    using io_callback = std::function<void(const boost::system::error_code &, std::size_t)>;
}