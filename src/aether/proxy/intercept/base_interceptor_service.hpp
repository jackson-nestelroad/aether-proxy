/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <map>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "aether/util/any_invocable.hpp"

namespace proxy::intercept {
using interceptor_id = std::size_t;
constexpr interceptor_id not_attached = 0;

namespace base_interceptor_service_detail {

template <bool B, typename... Ts>
class base_interceptor_service_impl {};

template <typename Event, typename... Args>
class base_interceptor_service_impl<true, Event, Args...> {
 public:
  using interceptor_func_t = util::any_invocable<void(Args...)>;
  using interceptor_func_ptr_t = void (*)(Args...);

  // Interceptors are stored in a two-dimensional map.
  // The first level is indexed by the event enumeration type.
  // The second level is indexed by interceptor ID.

  // This allows quick access to run all interceptors for a given event.

  // Another map is kept to map interceptor IDs to event, to allow quick deletion.
  // interceptor_lookup_.find(id) => interceptors_.find(event).delete(id).

  class functor {
   public:
    virtual void operator()(Args... args) = 0;
  };

  class service : public functor {
   public:
    virtual Event event() const = 0;
    virtual ~service() = default;
  };

  base_interceptor_service_impl() = default;
  ~base_interceptor_service_impl() = default;
  base_interceptor_service_impl(const base_interceptor_service_impl& other) = delete;
  base_interceptor_service_impl& operator=(const base_interceptor_service_impl& other) = delete;
  base_interceptor_service_impl(base_interceptor_service_impl&& other) noexcept = delete;
  base_interceptor_service_impl& operator=(base_interceptor_service_impl&& other) noexcept = delete;

  interceptor_id attach(Event ev, interceptor_func_t func) {
    const auto& event_map = interceptors_.find(ev);
    if (event_map == interceptors_.end()) {
      event_map_t map;
      map.emplace(next_id_, std::move(func));
      interceptors_.emplace(ev, std::move(map));
    } else {
      event_map->second.emplace(next_id_, std::move(func));
    }
    interceptor_lookup_.emplace(next_id_, ev);
    return next_id_++;
  }

  template <typename T>
  std::enable_if_t<std::is_base_of_v<typename base_interceptor_service_impl<true, Event, Args...>::service, T>,
                   interceptor_id>
  attach() {
    auto new_service = T();
    return attach(new_service.event(), std::move(new_service));
  }

  template <typename T>
  std::enable_if_t<std::is_base_of_v<typename base_interceptor_service_impl<true, Event, Args...>::functor, T>,
                   interceptor_id>
  attach(Event event) {
    return attach(event, T());
  }

  template <Event ev, typename T>
  std::enable_if_t<std::is_base_of_v<typename base_interceptor_service_impl<true, Event, Args...>::functor, T>,
                   interceptor_id>
  attach() {
    return attach(ev, T());
  }

  template <Event ev, interceptor_func_ptr_t func>
  interceptor_id attach() {
    return attach(ev, func);
  }

  void detach(interceptor_id id) {
    auto lookup = interceptor_lookup_.find(id);
    if (lookup != interceptor_lookup_.end()) {
      auto event_map = interceptors_.find(lookup->second);
      if (event_map != interceptors_.end()) {
        event_map->second.erase(id);
      }
      interceptor_lookup_.erase(lookup);
    }
  }

  void run(Event ev, Args... args) {
    auto events = interceptors_.find(ev);
    if (events != interceptors_.end()) {
      auto& event_map = events->second;
      for (auto& [id, func] : event_map) {
        func(args...);
      }
    }
  }

 protected:
  using event_map_t = std::map<interceptor_id, interceptor_func_t>;
  using interceptor_map_t = std::unordered_map<Event, event_map_t>;

 private:
  interceptor_id next_id_ = not_attached + 1;
  interceptor_map_t interceptors_;
  std::map<interceptor_id, Event> interceptor_lookup_;
};

}  // namespace base_interceptor_service_detail

template <typename Events, typename... Args>
class base_interceptor_service
    : public base_interceptor_service_detail::base_interceptor_service_impl<std::is_enum_v<Events>, Events, Args...> {};

}  // namespace proxy::intercept
