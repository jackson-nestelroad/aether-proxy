/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <vector>
#include <thread>
#include <memory>
#include <functional>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <aether/proxy/types.hpp>
#include <aether/proxy/error/exceptions.hpp>
#include <aether/util/console.hpp>

namespace proxy::concurrent {
    /*
        Class for managing multiple io_context instances and running them with a thread pool.
    */
    class io_context_pool
    {
    private:
        std::vector<std::unique_ptr<boost::asio::io_context>> io_contexts;

        // Each io_context has an associated work object to keep the io_context alive
        // even when there is no work to do
        std::vector<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> guards;

        std::vector<std::unique_ptr<std::thread>> thread_pool;

        // The index of the next io_context to give
        std::size_t next;

        // The number of io_contexts
        std::size_t size;

    public:
        io_context_pool(std::size_t size);
        io_context_pool() = delete;
        ~io_context_pool() = default;
        io_context_pool(const io_context_pool &other) = delete;
        io_context_pool &operator=(const io_context_pool &other) = delete;
        io_context_pool(io_context_pool &&other) noexcept = default;
        io_context_pool &operator=(io_context_pool &&other) noexcept = default;

        /*
            Runs a single io_context in a thread that starts at the function given.
        */
        void run(const std::function<void(boost::asio::io_context &ioc)> &thread_fun);

        /*
            Stops all io_contexts.
        */
        void stop();
        boost::asio::io_context &get_io_context();
    };
}