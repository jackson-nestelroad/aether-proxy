/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <optional>

#include <aether/proxy/connection/connection_flow.hpp>
#include <aether/proxy/tcp/http/message/request.hpp>
#include <aether/proxy/tcp/http/message/response.hpp>

namespace proxy::tcp::http {
    /*
        Owner of one HTTP request/response pair.
    */
    class exchange {
    private:
        request::ptr req;
        response::ptr res;

    public:
        exchange();

        /*
            Checks if a response object has been attached to the exchange.
            Call make_response() to ensure this returns true.
        */
        bool has_response() const;

        /*
            Returns a reference to the request data.
            Always exists.
        */
        request &get_request();

        /*
            Returns a reference to the response data.
            Throws error::http::no_response_exception if make_response() has not yet been called.
        */
        response &get_response();

        /*
            Creates a response object owned by the exchange.
            The new object is returned out to be added to.
        */
        response &make_response();

        /*
            Overwrites any existing response data with new data.
        */
        void set_response(const response &new_resp);
    };
}