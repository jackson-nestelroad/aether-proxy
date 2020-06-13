/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>

namespace util {
    /*
        Small utility class for blocking the current thread.
        Will unblock whenever unblock() is called.
    */
    class thread_blocker {
    private:
        std::condition_variable cv;
        std::mutex mutex;

    public:
        /*
            Blocks the current thread until unblock() is called.
        */
        void block();

        /*
            Unblocks all blocked threads.
        */
        void unblock();
    };
}