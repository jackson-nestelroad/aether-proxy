/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "exchange.hpp"

namespace proxy::tcp::http {
    exchange::exchange()
        : req(new request()),
        res()
    { }

    bool exchange::has_response() const {
        return static_cast<bool>(res);
    }

    request &exchange::get_request() {
        return *req;
    }

    response &exchange::get_response() {
        if (!res) {
            throw error::http::no_response_exception { };
        }
        return *res;
    }

    response &exchange::make_response() {
        res.reset(new response());
        return *res;
    }

    void exchange::set_response(const response &new_resp) {
        res = std::make_shared<response>(new_resp);
    }
}