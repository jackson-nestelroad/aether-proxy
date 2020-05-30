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
        Class for managing multiple io_service instances and running them with a thread pool.
    */
    class io_service_pool 
        : private boost::noncopyable {
    private:
        std::vector<io_service::ptr> io_services;

        // Each io_service has an associated work object to keep the io_service alive
        // even when there is no work to do
        std::vector<io_work::ptr> work;

        std::vector<std::unique_ptr<std::thread>> thread_pool;

        // The index of the next io_service to give
        std::size_t next;

        // The number of io_services
        std::size_t size;

    public:
        io_service_pool(std::size_t size);

        /*
            Runs a single io_service in a thread that starts at the function given.
        */
        void run(const std::function<void(io_service::ptr ios)> &thread_fun);

        /*
            Stops all io_services.
        */
        void stop();
        io_service::ptr get_io_service();
    };
}