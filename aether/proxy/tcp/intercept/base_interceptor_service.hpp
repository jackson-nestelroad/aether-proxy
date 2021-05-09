/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <map>
#include <unordered_map>
#include <functional>
#include <type_traits>

namespace proxy::tcp::intercept {
    using interceptor_id = std::size_t;
    constexpr interceptor_id not_attached = 0;

    namespace _meta {
        template <bool B, typename... Ts>
        class base_interceptor_service_impl { };

        template <typename Event, typename... Args>
        class base_interceptor_service_impl<true, Event, Args...> {
        public:
            using interceptor_func = std::function<void(Args...)>;
            using interceptor_func_ptr = void (*)(Args...);

            // Interceptors are stored in a two-dimensional map
            // The first level is indexed by the event enumeration type
            // The second level is indexed by interceptor ID

            // This allows quick access to run all interceptors for a given event
            
            // Another map is kept to map interceptor IDs to event, to allow quick deletion
            // interceptor_lookup.find(id) => interceptors.find(event).delete(id)

        protected:
            using event_map_t = std::map<interceptor_id, interceptor_func>;
            using interceptor_map_t = std::unordered_map<Event, event_map_t>;

        private:
            interceptor_id next_id = not_attached + 1;
            interceptor_map_t interceptors;
            std::map<interceptor_id, Event> interceptor_lookup;

        public:
            class functor {
            public:
                virtual void operator()(Args... args) = 0;
            };

            class service
                : public functor {
            public:
                virtual Event event() const = 0;
            };

            base_interceptor_service_impl() = default;
            ~base_interceptor_service_impl() = default;
            base_interceptor_service_impl(const base_interceptor_service_impl &other) = delete;
            base_interceptor_service_impl &operator=(const base_interceptor_service_impl &other) = delete;
            base_interceptor_service_impl(base_interceptor_service_impl &&other) noexcept = delete;
            base_interceptor_service_impl &operator=(base_interceptor_service_impl &&other) noexcept = delete;

            interceptor_id attach(Event ev, const interceptor_func &func) {
                const auto &event_map = interceptors.find(ev);
                if (event_map == interceptors.end()) {
                    interceptors.emplace(std::make_pair(ev, event_map_t { { next_id, func } }));
                }
                else {
                    event_map->second.emplace(next_id, func);
                }
                interceptor_lookup.emplace(next_id, ev);
                return next_id++;
            }

            template <typename T>
            std::enable_if_t<std::is_base_of_v<typename base_interceptor_service_impl<true, Event, Args...>::service, T>, interceptor_id>
            attach() {
                auto &&new_service = T();
                return attach(new_service.event(), std::move(new_service));
            }

            template <typename T>
            std::enable_if_t<std::is_base_of_v<typename base_interceptor_service_impl<true, Event, Args...>::functor, T>, interceptor_id>
            attach(Event event) {
                return attach(event, T());
            }

            template <Event ev, typename T>
            std::enable_if_t<std::is_base_of_v<typename base_interceptor_service_impl<true, Event, Args...>::functor, T>, interceptor_id>
                attach() {
                return attach(ev, T());
            }

            template <Event ev, interceptor_func_ptr func>
            interceptor_id attach() {
                return attach(ev, func);
            }

            void detach(interceptor_id id) {
                auto lookup = interceptor_lookup.find(id);
                if (lookup != interceptor_lookup.end()) {
                    auto event_map = interceptors.find(lookup->second);
                    if (event_map != interceptors.end()) {
                        event_map->second.erase(id);
                    }
                    interceptor_lookup.erase(lookup);
                }
            }

            void run(Event ev, Args... args) const {
                auto events = interceptors.find(ev);
                if (events != interceptors.end()) {
                    const auto &event_map = events->second;
                    for (const auto &[id, func] : event_map) {
                        func(args...);
                    }
                }
            }
        };
    }

    template <typename Events, typename... Args>
    class base_interceptor_service
        : public _meta::base_interceptor_service_impl<std::is_enum_v<Events>, Events, Args...>
    { };
}