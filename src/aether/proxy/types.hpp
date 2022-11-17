/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <type_traits>
#include <vector>

#include "aether/util/any_invocable.hpp"
#include "aether/util/bytes.hpp"
#include "aether/util/streambuf.hpp"

// Various definitions used across the codebase.

namespace proxy {

namespace types_detail {

// Mask implemenation universal case.
template <bool B, typename T, template <typename...> class SmartPtr>
struct mask_impl {};

// Mask implementation true case.
template <typename T, template <typename...> class SmartPtr>
struct mask_impl<true, T, SmartPtr> {
  using type = T;
  using ptr = SmartPtr<T>;
};

// Tests if a given class template is a smart pointer.
template <template <typename...> typename T>
struct is_smart_ptr : std::false_type {};
template <>
struct is_smart_ptr<std::unique_ptr> : std::true_type {};
template <>
struct is_smart_ptr<std::shared_ptr> : std::true_type {};
template <>
struct is_smart_ptr<std::weak_ptr> : std::true_type {};

// Masks an existing type in a struct that re-exposes its type and a smart pointer.
// Used for shortening the names of boost::asio:: types in a common format.
template <typename T, template <typename...> class SmartPtr>
struct mask : mask_impl<is_smart_ptr<SmartPtr>::value, T, SmartPtr> {};

}  // namespace types_detail

using port_t = unsigned short;
using milliseconds = boost::posix_time::milliseconds;
using streambuf = util::buffer::streambuf;
using const_buffer = util::buffer::const_buffer;

using byte_t = util::bytes::byte_t;
using double_byte_t = util::bytes::double_byte_t;
using byte_array_t = util::bytes::byte_array_t;

// Various types of callback functions
using callback_t = util::any_invocable<void()>;
using err_callback_t = util::any_invocable<void(const boost::system::error_code&)>;
using io_callback_t = util::any_invocable<void(const boost::system::error_code&, std::size_t)>;

}  // namespace proxy
