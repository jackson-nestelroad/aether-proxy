/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <map>
#include <functional>
#include <type_traits>

namespace proxy::tcp::intercept {
    namespace _meta {
        template <bool B, typename... Ts>
        class base_interceptor_service_impl { };

        template <typename Event, typename... Args>
        class base_interceptor_service_impl<true, Event, Args...> {
        public:
            using interceptor_func = std::function<void(Args...)>;
            using interceptor_id = std::size_t;

        private:
            using interceptor_pair = std::pair<Event, interceptor_func>;
            interceptor_id next_id = 0;
            std::map<interceptor_id, interceptor_pair> interceptors;

        public:
            interceptor_id attach(Event ev, const interceptor_func &func) {
                interceptors.insert({ next_id, { ev, func } });
                interceptor_id out = next_id++;
                return next_id;
            }

            void detach(interceptor_id id) {
                auto it = interceptors.find(id);
                interceptors.erase(it);
            }

            void run(Event ev, Args... args) const {
                for (const auto &[id, pair] : interceptors) {
                    if (pair.first == ev) {
                        pair.second(args...);
                    }
                }
            }

            class functor {
            public:
                virtual void operator()(Args... args) = 0;
            };

            class service 
                : public functor {
            public:
                virtual Event event() const = 0;
            };
        };
    }

    template <typename Events, typename... Args>
    class base_interceptor_service
        : public _meta::base_interceptor_service_impl<std::is_enum_v<Events>, Events, Args...>,
        private boost::noncopyable
    { };
}