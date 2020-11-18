/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <functional>
#include <csignal>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <aether/util/console.hpp>

namespace util {
    /*
        Class for handling exit signals.
        Runs its own io_context and thread, separate from the server.
    */
    class signal_handler {
    public:
        using callback = std::function<void()>;

    private:
        boost::asio::io_context &ioc;
        boost::asio::signal_set signals;
        bool paused;

        void on_signal(const boost::system::error_code &error, int signo, const callback &handler);

    public:
        signal_handler(boost::asio::io_context &ioc);
        ~signal_handler();
        void wait(const callback &handler);
        void pause();
        void unpause();
    };
}