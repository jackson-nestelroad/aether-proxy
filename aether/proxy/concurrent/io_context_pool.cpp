/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "io_context_pool.hpp"

namespace proxy::concurrent {
    io_context_pool::io_context_pool(std::size_t size) 
        : next(0),
        size(size)
    {
        if (size == 0) {
            throw error::invalid_option_exception("Number of threads cannot be 0");
        }

        for (std::size_t i = 0; i < size; ++i) {
            auto &new_service = io_contexts.emplace_back(std::make_unique<boost::asio::io_context>());
            guards.emplace_back(new_service->get_executor());
        }
    }

    void io_context_pool::run(const std::function<void(boost::asio::io_context &ioc)> &thread_fun) {
        for (std::size_t i = 0; i < size; ++i) {
            std::unique_ptr<std::thread> thr = std::make_unique<std::thread>(boost::bind(thread_fun, std::ref(*io_contexts[i])));
            thread_pool.push_back(std::move(thr));
        }
    }

    void io_context_pool::stop() {
        for (std::size_t i = 0; i < size; ++i) {
            io_contexts[i]->stop();
        }

        if (!thread_pool.empty()) {
            for (std::size_t i = 0; i < size; ++i) {
                thread_pool[i]->join();
            }
        }
    }

    boost::asio::io_context &io_context_pool::get_io_context() {
        auto &out = io_contexts[next];
        next = (next + 1) % size;
        return *out;
    }
}