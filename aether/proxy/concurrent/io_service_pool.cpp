/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "io_service_pool.hpp"

namespace proxy::concurrent {
    io_service_pool::io_service_pool(std::size_t size) 
        : next(0),
        size(size)
    {
        if (size == 0) {
            throw error::invalid_option_exception("Number of threads cannot be 0");
        }

        for (std::size_t i = 0; i < size; ++i) {
            io_service::ptr new_service(new io_service::type());
            io_work::ptr new_work(new io_work::type(*new_service));
            io_services.push_back(new_service);
            work.push_back(new_work);
        }
    }

    void io_service_pool::run(const std::function<void(io_service::ptr ios)> &thread_fun) {
        for (std::size_t i = 0; i < size; ++i) {
            std::unique_ptr<std::thread> thr(
                new std::thread(boost::bind(thread_fun, io_services[i]))
            );
            thread_pool.push_back(std::move(thr));
        }
    }

    void io_service_pool::stop() {
        for (std::size_t i = 0; i < size; ++i) {
            io_services[i]->stop();
        }

        for (std::size_t i = 0; i < size; ++i) {
            thread_pool[i]->join();
        }
    }

    io_service::ptr io_service_pool::get_io_service() {
        auto out = io_services[next];
        next = (next + 1) % size;
        return out;
    }
}