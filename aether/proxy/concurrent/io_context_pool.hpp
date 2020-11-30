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
#include <boost/noncopyable.hpp>

#include <aether/proxy/types.hpp>
#include <aether/proxy/error/exceptions.hpp>
#include <aether/util/console.hpp>

namespace proxy::concurrent {
    /*
        Class for managing multiple io_context instances and running them with a thread pool.
    */
    class io_context_pool 
        : private boost::noncopyable {
    private:
        std::vector<std::unique_ptr<boost::asio::io_context>> io_contexts;

        // Each io_context has an associated work object to keep the io_context alive
        // even when there is no work to do
        std::vector<boost::asio::io_context::work> work;

        std::vector<std::unique_ptr<std::thread>> thread_pool;

        // The index of the next io_context to give
        std::size_t next;

        // The number of io_contexts
        std::size_t size;

    public:
        io_context_pool(std::size_t size);

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