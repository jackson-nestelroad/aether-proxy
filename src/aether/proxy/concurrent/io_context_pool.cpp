/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "io_context_pool.hpp"

#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "aether/proxy/error/error.hpp"
#include "aether/util/console.hpp"

namespace proxy::concurrent {

result<io_context_pool> io_context_pool::create(std::size_t size) {
  if (size == 0) {
    return error::invalid_option("Number of threads cannot be 0");
  }

  return io_context_pool(size);
}

io_context_pool::io_context_pool(std::size_t size) : next_(0), size_(size) {
  for (std::size_t i = 0; i < size_; ++i) {
    auto& new_service = io_contexts_.emplace_back(std::make_unique<boost::asio::io_context>());
    guards_.emplace_back(new_service->get_executor());
  }
}

void io_context_pool::run(const std::function<void(boost::asio::io_context& ioc)>& thread_fun) {
  for (std::size_t i = 0; i < size_; ++i) {
    boost::asio::io_context& io_context = *io_contexts_[i];
    std::unique_ptr<std::thread> thr =
        std::make_unique<std::thread>([thread_fun, &io_context] { thread_fun(io_context); });
    thread_pool_.push_back(std::move(thr));
  }
}

void io_context_pool::stop() {
  for (std::size_t i = 0; i < size_; ++i) {
    io_contexts_[i]->stop();
  }

  if (!thread_pool_.empty()) {
    for (std::size_t i = 0; i < size_; ++i) {
      thread_pool_[i]->join();
    }
  }
}

boost::asio::io_context& io_context_pool::get_io_context() {
  boost::asio::io_context& out = *io_contexts_[next_];
  ++next_;
  if (next_ >= size_) {
    next_ = 0;
  }
  return out;
}

}  // namespace proxy::concurrent
