/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/tcp/base_service.hpp>
#include <aether/proxy/tcp/websocket/pipeline.hpp>
#include <aether/proxy/tcp/websocket/protocol/websocket_manager.hpp>

namespace proxy::tcp::websocket {
    /*
        Service for handling WebSocket connections.
    */
    class websocket_service
        : public base_service
    {
    public:
        // Chunks are 4 KB
        // Client has a 4-byte mask key

        static constexpr std::size_t client_chunk_size = 4092 - 4;
        static constexpr std::size_t server_chunk_size = 4092;
    
    private:
        /*
            Data structure representing one end of a WebSocket connection.
            Used to run the same code on both ends of the connection simultaneously.
            Only read from source and only write to destination.
        */
        struct websocket_connection {
            endpoint source_ep;
            endpoint destination_ep;
            connection::base_connection &source;
            connection::base_connection &destination;
            protocol::websocket_manager &manager;
            std::atomic<bool> finished = false;
            bool ready_to_intercept = true;
            std::string next_message_content;
        };

        pipeline pline;
        
        protocol::websocket_manager client_manager;
        protocol::websocket_manager server_manager;

        websocket_connection client_connection;
        websocket_connection server_connection;

        constexpr websocket_connection &get_connection_for_endpoint(endpoint ep) {
            return ep == endpoint::client ? client_connection : server_connection;
        }

        void websocket_loop(websocket_connection &connection);
        void on_destination_write(const boost::system::error_code &error, std::size_t bytes_transferred, websocket_connection &connection);
        void on_source_read(const boost::system::error_code &error, std::size_t bytes_transferred, websocket_connection &connection);
        void on_error(const boost::system::error_code &error, websocket_connection &connection);
        void close_connection(websocket_connection &connection);
        void on_close(const boost::system::error_code &error, std::size_t bytes_transferred, websocket_connection &connection);
        void finish_connection(websocket_connection &connection);
        void on_finish();

        void on_ping_frame(websocket_connection &connection, ping_frame &&frame);
        void on_pong_frame(websocket_connection &connection, pong_frame &&frame);
        void on_close_frame(websocket_connection &connection, close_frame &&frame);
        void on_message_frame(websocket_connection &connection, message_frame &&frame);
        void send_message(websocket_connection &connection, message &&msg);

    public:
        websocket_service(connection::connection_flow &flow, connection_handler &owner,
            server_components &components, http::exchange &handshake);

        void start() override;
    };
}