/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "server.hpp"

namespace proxy {
    server::server(const program::options &options)
        : options(options),
        io_services(options.thread_pool_size),
        is_running(false)
    { }

    server::~server() {
        if (is_running) {
            stop();
        }
    }

    void server::run_service(io_service::ptr ios) {
        while (true) {
            try {
                ios->run();
                break;
            }
            catch (const error::base_exception &ex) {
                out::error::log(ex.what());
            }
        }
    }

    void server::start() {
        is_running = true;
        acc.reset(new acceptor(io_services, options));
        acc->start();
        io_services.run(run_service);
    }

    void server::stop() {
        is_running = false;
        acc->stop();
        io_services.stop();
    }

    std::string server::endpoint_string() const {
        if (!acc) {
            throw error::invalid_operation_exception { "Cannot access port before server has started. Call server.start(options) first." };
        }
        std::stringstream str;
        str << acc->get_endpoint();
        return str.str();
    }
}