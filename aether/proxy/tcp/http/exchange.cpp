/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "exchange.hpp"

namespace proxy::tcp::http {
    http::request &exchange::request() {
        return req;
    }

    const http::request &exchange::request() const {
        return req;
    }

    http::response &exchange::response() {
        if (!has_response()) {
            throw error::http::no_response_exception { no_response_error_message.data() };
        }
        return res.value();
    }

    const http::response &exchange::response() const {
        if (!has_response()) {
            throw error::http::no_response_exception { no_response_error_message.data() };
        }
        return res.value();
    }

    http::response &exchange::make_response() {
        if (!has_response()) {
            return res.emplace();
        }
        return response();
    }

    void exchange::set_response(const http::response &res) {
        this->res = res;
    }

    bool exchange::has_response() const {
        return res.has_value();
    }

    void exchange::set_mask_connect(bool val) {
        mask_connect_flag = val;
    }

    bool exchange::mask_connect() const {
        return mask_connect_flag;
    }
}