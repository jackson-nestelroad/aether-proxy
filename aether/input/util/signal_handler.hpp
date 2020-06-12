/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <functional>
#include <csignal>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <aether/proxy/types.hpp>
#include <aether/util/console.hpp>

namespace input::util {
    /*
        Class for handling exit signals.
        Runs its own io_service and thread, separate from the server.
    */
    class signal_handler {
    private:
        boost::asio::io_service &ios;
        boost::asio::signal_set signals;
        bool paused;

        void on_signal(const boost::system::error_code &error, int signo, const proxy::callback &handler);

    public:
        signal_handler(boost::asio::io_service &ios);
        ~signal_handler();
        void wait(const proxy::callback &handler);
        void pause();
        void unpause();
    };
}