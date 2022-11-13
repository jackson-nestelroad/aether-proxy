/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>

namespace util {

// Small utility class for blocking the current thread.
// Will unblock whenever unblock() is called.
class thread_blocker {
 public:
  // Blocks the current thread until unblock() is called.
  void block();

  // Unblocks all blocked threads.
  void unblock();

 private:
  std::condition_variable cv_;
  std::mutex mutex_;
};
}  // namespace util
