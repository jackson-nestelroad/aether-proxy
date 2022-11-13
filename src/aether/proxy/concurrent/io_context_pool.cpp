/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "io_context_pool.hpp"

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "aether/proxy/error/exceptions.hpp"
#include "aether/util/console.hpp"

namespace proxy::concurrent {
io_context_pool::io_context_pool(std::size_t size) : next_(0), size_(size) {
  if (size_ == 0) {
    throw error::invalid_option_exception("Number of threads cannot be 0");
  }

  for (std::size_t i = 0; i < size_; ++i) {
    auto& new_service = io_contexts_.emplace_back(std::make_unique<boost::asio::io_context>());
    guards_.emplace_back(new_service->get_executor());
  }
}

void io_context_pool::run(const std::function<void(boost::asio::io_context& ioc)>& thread_fun) {
  for (std::size_t i = 0; i < size_; ++i) {
    std::unique_ptr<std::thread> thr =
        std::make_unique<std::thread>(boost::bind(thread_fun, std::ref(*io_contexts_[i])));
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
  auto& out = io_contexts_[next_];
  ++next_;
  if (next_ >= size_) {
    next_ = 0;
  }
  return *out;
}

}  // namespace proxy::concurrent
