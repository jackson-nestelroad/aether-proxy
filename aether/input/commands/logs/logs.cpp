/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "logs.hpp"

namespace input::commands {
    void logs::run(const arguments &args, proxy::server &server, command_service &caller) {
        out::console::log("Logs started. Press Ctrl+C (^C) to stop logging.");
        attach_interceptors(server);
        signals.reset(new util::signal_handler(server.get_io_context()));
        signals->wait([this]() { blocker.unblock(); });
        blocker.block();
        detach_interceptors(server);
        signals.reset();
    }

    void logs::attach_interceptors(proxy::server &server) {
        constexpr std::ostream *strm = &std::cout;
        http_id = interceptors::attach_http_interceptor<interceptors::http::http_logger<strm>>(server);
    }

    void logs::detach_interceptors(proxy::server &server) {
        server.interceptors.http.detach(http_id);
    }
}