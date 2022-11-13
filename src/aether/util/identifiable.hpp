/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <mutex>
#include <type_traits>

namespace util::id {

namespace id_detail {

template <bool B, typename T>
class identifiable;

template <typename T>
class identifiable<true, T> {
 public:
  using id_t = T;

  id_t id() const { return id_; }

 protected:
  identifiable() : id_(next_id()) {}

  const id_t id_;

 private:
  static T next_id() {
    static T next_id{};
    static std::mutex id_mutex{};

    std::lock_guard<std::mutex> lock(id_mutex);
    return next_id++;
  }
};

template <typename T, typename = T>
struct incrementable : std::false_type {};

template <typename T>
struct incrementable<T, decltype(std::declval<T&>()++)> : std::true_type {};

}  // namespace id_detail

// Automatically generates a unique identifier for each instance of deriving classes.
template <typename T>
using identifiable =
    id_detail::identifiable<id_detail::incrementable<T>::value && std::is_default_constructible<T>::value, T>;

}  // namespace util::id
