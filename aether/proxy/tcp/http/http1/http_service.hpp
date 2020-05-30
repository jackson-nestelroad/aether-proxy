/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <memory>
#include <boost/asio.hpp>

#include <aether/proxy/types.hpp>
#include <aether/proxy/config/config.hpp>
#include <aether/proxy/base_service.hpp>
#include <aether/proxy/connection/connection_flow.hpp>
#include <aether/proxy/tcp/http/exchange.hpp>
#include <aether/proxy/tcp/util/buffer.hpp>
#include <aether/proxy/tcp/http/http1/parser.hpp>
#include <aether/proxy/tcp/websocket/handshake.hpp>
#include <aether/proxy/tcp/tunnel/tunnel_service.hpp>

namespace proxy::tcp::http::http1 {
    /*
        Service for handling HTTP/1.x connections.
    */
    class http_service
        : public base_service {
    private:
        // Static continue response used whenever client expects it
        static const response continue_response;
        
        // Static response to CONNECT requests used whenever client needs it
        static const response connect_response;

        exchange exch;
        parser _parser;

        // Methods are quite broken up because socket operations are asynchronous

        void read_request_head();
        void on_read_request_head(const boost::system::error_code &error, std::size_t bytes_transferred);
        void read_request_body(const callback &handler);
        void on_read_request_body(const callback &handler, const boost::system::error_code &error, std::size_t bytes_transferred);
        void handle_request();
        void connect_server();
        void on_connect_server(const boost::system::error_code &error);
        void forward_request();
        void on_forward_request(const boost::system::error_code &error, std::size_t bytes_transferred);
        void read_response_head();
        void on_read_response_head(const boost::system::error_code &error, std::size_t bytes_transferred);
        void read_response_body(const callback &handler);
        void on_read_response_body(const callback &handler, const boost::system::error_code &error, std::size_t bytes_transferred);
        void forward_response();
        void on_forward_response(const boost::system::error_code &error, std::size_t bytes_transferred);
        void handle_response();

        void send_connect_response();
        void on_send_connect_response(const boost::system::error_code &error, std::size_t bytes_transferred);

        /*
            Modifies the request target if needed for transmission.
            Host information is kept primarily in the request.target.netloc field.
        */
        void validate_target();

        /*
            Sends an HTML error response to the client.
            Overwrites any previous response data in the HTTP exchange.
        */
        void send_error_response(status response_code, std::string_view msg = "No message given");
        void on_write_error_response(const boost::system::error_code &error, std::size_t bytes_transferred);

    public:
        http_service(connection::connection_flow::ptr flow, connection_handler &owner);
        void start() override;
        exchange get_exchange() const;
    };
}