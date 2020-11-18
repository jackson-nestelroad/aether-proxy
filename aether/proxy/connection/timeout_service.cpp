/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "timeout_service.hpp"

namespace proxy::connection {
    timeout_service::timeout_service(boost::asio::io_context &ioc)
        : ioc(ioc),
        timer(ioc)
    { }

    void timeout_service::reset_timer() {
        timer.expires_from_now(boost::posix_time::pos_infin);
    }

    void timeout_service::on_timeout(const callback &handler, const boost::system::error_code &error) {
        // Timer was not canceled
        if (error != boost::asio::error::operation_aborted) {
            reset_timer();
            boost::asio::post(ioc, handler);
        }
    }

    void timeout_service::set_timeout(const milliseconds &time, const callback &handler) {
        timer.expires_from_now(time);
        timer.async_wait(boost::bind(&timeout_service::on_timeout, this, handler, boost::asio::placeholders::error));
    }

    void timeout_service::cancel_timeout() {
        timer.cancel();
        reset_timer();
    }
}