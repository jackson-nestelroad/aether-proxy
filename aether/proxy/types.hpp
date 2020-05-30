/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <array>
#include <functional>
#include <memory>
#include <type_traits>
#include <boost/asio.hpp>

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

    // Various types of callback functions
    using callback = std::function<void()>;
    using err_callback = std::function<void(const boost::system::error_code &)>;
    using io_callback = std::function<void(const boost::system::error_code &, std::size_t)>;

    using socket = _meta::_mask<boost::asio::ip::tcp::socket, std::shared_ptr>;
    using io_service = _meta::_mask<boost::asio::io_service, std::shared_ptr>;
    using io_work = _meta::_mask<boost::asio::io_service::work, std::shared_ptr>;
}