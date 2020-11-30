/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "error_state.hpp"

namespace proxy::error {
    error_state::error_state() noexcept 
        : boost_error_code(), proxy_error_code(), message() 
    { }

    void error_state::set_proxy_error(const base_exception &ex) noexcept {
        proxy_error_code = ex.error_code();
        message = ex.what();
    }

    bool error_state::has_message() const noexcept {
        return !message.empty();
    }

    std::string_view error_state::get_message() const noexcept {
        return message;
    }

    void error_state::set_message(std::string_view msg) noexcept {
        message = msg;
    }

    std::string_view error_state::get_message_or_proxy() const noexcept {
        return message.empty() ? proxy_error_code.message() : message;
    }

    std::string_view error_state::get_message_or_boost() const noexcept {
        return message.empty() ? boost_error_code.message() : message;
    }
}