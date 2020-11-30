/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <mutex>
#include <type_traits>

namespace util::id {
    namespace _impl {
        template <bool B, typename T>
        class identifiable;

        template <typename T>
        class identifiable<true, T> {
        public:
            using id_t = T;

        private:
            static T next_id() {
                static T next_id { };
                static std::mutex id_mutex { };

                std::lock_guard<std::mutex> lock(id_mutex);
                return next_id++;
            }

        protected:
            const id_t _id;

            identifiable() 
                : _id(next_id())
            { }

        public:
            id_t id() const {
                return _id;
            }
        };

        template <typename T, typename = T>
        struct incrementable : std::false_type { };

        template <typename T>
        struct incrementable<T, decltype(std::declval<T &>()++)> : std::true_type { };
    }

    /*
        Automatically generates a unique identifier for each instance of deriving classes.
    */
    template <typename T>
    using identifiable = _impl::identifiable<_impl::incrementable<T>::value && std::is_default_constructible<T>::value, T>;
}