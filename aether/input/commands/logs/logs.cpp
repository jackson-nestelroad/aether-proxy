/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "logs.hpp"

namespace input::commands {
    void logs::run(const arguments &args, proxy::server &server, command_service &caller) {
        running = true;
        out::console::log("Logs started. Press Ctrl+C (^C) to stop.");
        attach_interceptors(server);
        signals.reset(new util::signal_handler(caller.io_service()));
    }

    void logs::attach_interceptors(proxy::server &server) {
        constexpr std::ostream *strm = &std::cout;
        interceptors::attach_http_interceptor<interceptors::http::http_logger<strm>>(server);
    }

    void logs::stop() {
        running = false;
    }
}