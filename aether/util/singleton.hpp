/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <mutex>

namespace util {
    namespace _meta {
        template <typename T, typename = void>
        struct has_init : std::false_type { };

        template <typename T>
        struct has_init<T, decltype(std::declval<T>().init())> : std::true_type { };

        template <typename T, typename = void>
        struct has_cleanup : std::false_type { };

        template <typename T>
        struct has_cleanup<T, decltype(std::declval<T>().cleanup())> : std::true_type { };
    }

    /*
        Class for a lazy-loading singleton. The singleton instance is created, and initialized,
            when it is needed for the first time.
        The deriving class can add init() and cleanup() methods to work with the singleton.
    */
    template <typename Self, typename Return>
    class base_singleton {
    private:
        static std::unique_ptr<Self> single_instance;
        static std::once_flag init_flag;

        static void init_single_instance() {
            single_instance = std::make_unique<Self>();
            if constexpr (_meta::has_init<Self>::value) {
                single_instance->init();
            }
        }

    protected:
        /*
            Access the singleton instance internally.
        */
        static Self &raw_instance() {
            std::call_once(init_flag, &init_single_instance);
            return *single_instance;
        }

        base_singleton() = default;

        ~base_singleton() {
            if constexpr (_meta::has_cleanup<Self>::value) {
                single_instance->cleanup();
            }
        }

        base_singleton(const base_singleton &other) = delete;
        base_singleton &operator=(const base_singleton &other) = delete;
        base_singleton(base_singleton &&other) noexcept = delete;
        base_singleton &operator=(base_singleton &&other) noexcept = delete;

    public:
        /*
            Access the singleton instance externally.
        */
        static Return &instance() {
            std::call_once(init_flag, &init_single_instance);
            return *single_instance;
        }
    };

    template <typename Self, typename Return>
    std::unique_ptr<Self> base_singleton<Self, Return>::single_instance { };

    template <typename Self, typename Return>
    std::once_flag base_singleton<Self, Return>::init_flag;

    template <typename Self>
    class singleton
        : public base_singleton<Self, Self> { };

    template <typename Self>
    class const_singleton
        : public base_singleton<Self, const Self> { };
}