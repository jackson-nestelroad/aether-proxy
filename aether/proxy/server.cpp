/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "server.hpp"

namespace proxy {
    server::server(const program::options &options)
        : components(options),
        interceptors(this->components.interceptors),
        is_running(false),
        needs_cleanup(false),
        log_manager()
    {
        log_manager.unsync_with_stdio();

        // All logs are silenced in interactive mode until manually unsilenced
        if (components.options.run_interactive) {
            log_manager.silence();
        }

        // Logs may be redirected to a log file, even in command service mode
        if (!components.options.log_file_name.empty()) {
            log_manager.redirect_to_file(components.options.log_file_name);
        }

        // The silent option overrides any log file setting
        if (components.options.run_silent) {
            log_manager.silence();
        }
    }

    server::~server() {
        stop();
    }

    void server::run_service(boost::asio::io_context &ioc) {
        while (true) {
            try {
                ioc.run();
                break;
            }
            catch (const std::exception &ex) {
                out::safe_error::log("Unexpected error running io_context instance:", ex.what());
            }
        }
    }

    void server::start() {
        out::debug::log("Starting server");

        is_running = true;
        needs_cleanup = true;
        signals = std::make_unique<util::signal_handler>(components.io_contexts.get_io_context());

        signals->wait(boost::bind(&server::signal_stop, this));

        acc = std::make_unique<acceptor>(components);
        acc->start();
        components.io_contexts.run(run_service);
    }

    // When server is stopped with signals, it stops running but is not cleaned up here
    // Since the signal handler is running on the same threads as the server, we 
    // cannot clean up here without a resource dead lock
    // Thus, the needs_cleanup flag signals another thread to clean things up
    void server::signal_stop() {
        is_running = false;
        blocker.unblock();
    }

    // Stops the server and calls cleanup methods
    void server::stop() {
        signal_stop();
        cleanup();
    }

    // Cleans up the server, if necessary
    void server::cleanup() {
        if (needs_cleanup) {
            if (acc) {
                acc->stop();
            }
            components.io_contexts.stop();
            signals.reset();
            blocker.unblock();
            needs_cleanup = false;
        }
    }

    void server::await_stop() {
        if (is_running) {
            blocker.block();
        }
        cleanup();
    }

    void server::pause_signals() {
        if (!signals) {
            throw error::invalid_operation_exception { "Cannot pause signals when server is not running." };
        }
        signals->pause();
    }

    void server::unpause_signals() {
        if (!signals) {
            throw error::invalid_operation_exception { "Cannot unpause signals when server is not running." };
        }
        signals->unpause();
    }

    void server::enable_logs() {
        log_manager.unsilence();
    }

    void server::disable_logs() {
        log_manager.silence();
    }

    bool server::running() const {
        return is_running;
    }

    std::string server::endpoint_string() const {
        if (!acc) {
            throw error::invalid_operation_exception { "Cannot access port before server has started. Call server.start() first." };
        }
        std::stringstream str;
        str << acc->get_endpoint();
        return str.str();
    }

    boost::asio::io_context &server::get_io_context() {
        return components.io_contexts.get_io_context();
    }
}