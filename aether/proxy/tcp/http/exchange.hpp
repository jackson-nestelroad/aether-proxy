/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <optional>

#include <aether/proxy/tcp/http/message/request.hpp>
#include <aether/proxy/tcp/http/message/response.hpp>

namespace proxy::tcp::http {
    /*
        Owner of one HTTP request/response pair.
    */
    class exchange {
    private:
        static constexpr std::string_view no_response_error_message = "No response object in the HTTP exchange. Assure exchange::make_response or exchange::set_response is called before accessing the response object.";
        
        http::request req;
        std::optional<http::response> res;

        bool mask_connect_flag = false;

    public:
        http::request &request();
        const http::request &request() const;
        http::response &response();
        const http::response &response() const;

        /*
            Creates an empty response within the exchange pair.
        */
        http::response &make_response();

        /*
            Sets the response in the exchange pair.
        */
        void set_response(const http::response &res);

        /*
            Checks if the exchange has any response set.
        */
        bool has_response() const;

        /*
            Sets if the HTTP exchange should mask a CONNECT request.
            This is useful if the proxy knows that the CONNECT will only
                lead to more HTTP requests.
            This is extremely dangerous, and it will likely lead to an incorrect
                service being selected.
        */
        void set_mask_connect(bool val);

        /*
            Returns if the exchange should mask a CONNECT request, treating it
                like a normal HTTP request.
        */
        bool mask_connect() const;
    };
}