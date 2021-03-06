/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "signal_handler.hpp"

namespace util {
    signal_handler::signal_handler(boost::asio::io_context &ioc)
        : ioc(ioc),
        signals(ioc),
        paused(false)
    {
        signals.add(SIGINT);
        signals.add(SIGTERM);
#if defined(SIGQUIT)
        signals.add(SIGQUIT);
#endif
    }

    signal_handler::~signal_handler() {
        signals.cancel();
    }

    void signal_handler::wait(const callback &callback) {
        signals.async_wait(boost::bind(&signal_handler::on_signal, this,
            boost::asio::placeholders::error, boost::asio::placeholders::signal_number, callback));
    }

    void signal_handler::pause() {
        paused = true;
    }

    void signal_handler::unpause() {
        paused = false;
    }

    void signal_handler::on_signal(const boost::system::error_code &error, int signo, const callback &handler) {
        if (error == boost::system::errc::success) {
            if (paused) {
                wait(handler);
            }
            else {
                handler();
            }
        }
    }
}