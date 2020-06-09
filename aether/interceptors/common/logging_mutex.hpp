/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>
#include <thread>

namespace interceptors::common {
    /*
        Static std::mutex for a given std::ostream instance.
        Mutexes are NOT copyable or movable, so logging services must inherit the static
            mutex from this class.
        This is also how multiple logging services can print to the same output stream
            across multiple threads and services.
    */
    template <std::ostream *strm>
    class logging_mutex {
    protected:
        static std::mutex log_mutex;

    public:
        void lock() {
            this->log_mutex.lock();
        }

        void unlock() {
            this->log_mutex.unlock();
        }
    };

    template <std::ostream *strm>
    std::mutex logging_mutex<strm>::log_mutex { };
}