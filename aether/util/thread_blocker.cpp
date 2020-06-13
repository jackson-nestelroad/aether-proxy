/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "thread_blocker.hpp"

namespace util {
    void thread_blocker::block() {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock);
    }

    void thread_blocker::unblock() {
        cv.notify_all();
    }
}