/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include "websocket_service.hpp"

namespace proxy::tcp::websocket {
    websocket_service::websocket_service(connection::connection_flow &flow, connection_handler &owner,
        tcp::intercept::interceptor_manager &interceptors, http::exchange &handshake) 
        : base_service(flow, owner, interceptors),
        pline(handshake, program::options::instance().websocket_intercept_messages_by_default),
        client_manager(endpoint::client, pline.get_extensions()),
        server_manager(endpoint::server, pline.get_extensions()),
        client_connection {
            endpoint::server,
            endpoint::client,
            static_cast<connection::base_connection &>(flow.server),
            static_cast<connection::base_connection &>(flow.client),
            client_manager
        },
        server_connection {
            endpoint::client,
            endpoint::server,
            static_cast<connection::base_connection &>(flow.client),
            static_cast<connection::base_connection &>(flow.server),
            server_manager
        }
    { }

    void websocket_service::start() {
        interceptors.websocket.run(intercept::websocket_event::start, flow, pline);

        // Run the two connection loops
        // They start in the same thread but then diverge to operate asynchronously
        websocket_loop(client_connection);
        websocket_loop(server_connection);
    }

    void websocket_service::websocket_loop(websocket_connection &connection) {
        // Connection closed, and it closed from the other end
        // Close the connection from this end
        if (pline.closed()) {
            close_connection(connection);
        }
        else {
            try {
                // Parse input buffer
                std::optional<close_code> close;
                std::vector<completed_frame> &&frames = connection.manager.parse(connection.source.input_buffer(), close);
                
                // Close connection if parser indicates
                if (close.has_value()) {
                    pline.set_close_state(connection.source_ep, { close.value() });
                    close_connection(connection);
                }
                else {
                    // Handle frames accordingly
                    for (completed_frame &frame : frames) {
                        switch (frame.get_type()) {
                            case opcode::ping: on_ping_frame(connection, std::move(frame.get_ping_frame())); break;
                            case opcode::pong: on_pong_frame(connection, std::move(frame.get_pong_frame())); break;
                            case opcode::close: on_close_frame(connection, std::move(frame.get_close_frame())); break;
                            case opcode::binary:
                            case opcode::text: on_message_frame(connection, std::move(frame.get_message_frame())); break;
                        }
                    }

                    // Connection was just closed by an event handler
                    if (pline.closed()) {
                        close_connection(connection);
                        return;
                    }

                    // Inject frames
                    while (pline.has_frame(connection.destination_ep)) {
                        connection.manager.serialize(connection.destination.output_buffer(), std::move(pline.next_frame(connection.destination_ep)));
                        pline.pop_frame(connection.destination_ep);
                    }

                    // Inject messages
                    while (pline.has_message(connection.destination_ep)) {
                        send_message(connection, std::move(pline.next_message(connection.destination_ep)));
                        pline.pop_message(connection.destination_ep);
                    }

                    connection.destination.write_untimed_async(boost::bind(&websocket_service::on_destination_write, this,
                        boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, std::ref(connection)));
                }
            }
            // Parser may throw an exception, indicating an abnormal closure
            catch (const error::websocket_exception &error) {
                flow.error.set_proxy_error(error);
                pline.set_close_state(connection.source_ep, { close_code::protocol_error, error.what() });
                interceptors.websocket.run(intercept::websocket_event::error, flow, pline);
                close_connection(connection);
            }
        }
    }

    void websocket_service::on_ping_frame(websocket_connection &connection, ping_frame &&frame) {
        // Inject a pong frame back to the source
        pline.inject_frame(connection.source_ep, frame.response());

        // Send a ping frame to the destination
        connection.manager.serialize(connection.destination.output_buffer(), std::move(frame));
    }

    void websocket_service::on_pong_frame(websocket_connection &connection, pong_frame &&frame) {
        // Do nothing
    }

    void websocket_service::on_close_frame(websocket_connection &connection, close_frame &&frame) {
        // We only need to set the close state for the pipeline
        // When both sides move to the next iteration of websocket_loop, the connection
        // will see that the pipeline is closed and send the appropriate close frame.
        pline.set_close_state(connection.source_ep, frame);
    }

    void websocket_service::on_message_frame(websocket_connection &connection, message_frame &&frame) {
        if (pline.should_intercept()) {
            // Connection was set to intercept mid-way through a message, wait until the message finishes
            if (!connection.ready_to_intercept) {
                if (frame.finished) {
                    connection.ready_to_intercept = true;
                }
            }
            else {
                connection.next_message_content += std::move(frame.payload);

                if (frame.finished) {
                    message msg { frame.type, connection.source_ep };
                    msg.set_content(std::move(connection.next_message_content));
                    connection.next_message_content.clear();

                    // Message may be altered or blocked
                    interceptors.websocket_message.run(intercept::websocket_message_event::received, flow, pline, msg);

                    send_message(connection, std::move(msg));
                }
            }
        }
        else {
            connection.ready_to_intercept = false;
            connection.manager.serialize(connection.destination.output_buffer(), std::move(frame));
        }
    }

    void websocket_service::send_message(websocket_connection &connection, message &&msg) {
        if (!msg.blocked()) {
            std::string_view content = msg.get_content();
            std::size_t chunk_size = connection.source_ep == endpoint::client ? client_chunk_size : server_chunk_size;
            std::size_t content_length = content.length();
            std::size_t i = 0;
            bool finished = false;
            do {
                finished = i + chunk_size >= content_length;
                connection.manager.serialize(connection.destination.output_buffer(), message_frame { msg.get_type(), finished, std::string(content.substr(i, chunk_size)) });
                i += chunk_size;
            } while (!finished);
        }
    }

    void websocket_service::on_destination_write(const boost::system::error_code &error, std::size_t bytes_transferred, websocket_connection &connection) {
        // Error writing data out
        if (error != boost::system::errc::success) {
            on_error(error, connection);
        }
        else {
            connection.source.read_async(boost::bind(&websocket_service::on_source_read, this,
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, std::ref(connection)));
        }
    }

    void websocket_service::on_source_read(const boost::system::error_code &error, std::size_t bytes_transferred, websocket_connection &connection) {
        // Error reading new data, close the connection
        if (error != boost::system::errc::success) {
            // Looks like cancel() was called and the pipeline is closed, so close the connection
            if (error == boost::asio::error::operation_aborted && pline.closed()) {
                close_connection(connection);
            }
            // Some other error occurred
            else {
                on_error(error, connection);
            }
        }
        else {
            websocket_loop(connection);
        }
    }

    void websocket_service::on_error(const boost::system::error_code &error, websocket_connection &connection) {
        if (!pline.closed()) {
            pline.set_close_state(connection.source_ep, { close_code::internal_error, error.message() });
        }
        flow.error.set_boost_error(error);
        interceptors.websocket.run(intercept::websocket_event::error, flow, pline);

        // Looks like the connection closed, no use trying to send a close frame
        if (error == boost::asio::error::eof) {
            finish_connection(connection);
        }
        else {
            close_connection(connection);
        }
    }

    void websocket_service::close_connection(websocket_connection &connection) {
        if (!connection.finished) {
            try {
                connection.manager.serialize(connection.destination.output_buffer(), pline.get_close_frame());
                connection.destination.write_untimed_async(boost::bind(&websocket_service::on_close, this,
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, std::ref(connection)));
            }
            catch (const error::websocket::serialization_error_exception &error) {
                flow.error.set_proxy_error(error);
                interceptors.websocket.run(intercept::websocket_event::error, flow, pline);
                
                // A serialization error when closing the connection cannot really be recovered from
                finish_connection(connection);
            }
        }
        // Connection has already been properly closed, just go straight to finish handler
        else {
            on_finish();
        }
    }

    void websocket_service::on_close(const boost::system::error_code &error, std::size_t bytes_transferred, websocket_connection &connection) {
        if (error != boost::system::errc::success) {
            flow.error.set_boost_error(error);
            interceptors.websocket.run(intercept::websocket_event::error, flow, pline);
        }
        finish_connection(connection);
    }

    void websocket_service::finish_connection(websocket_connection &connection) {
        // Cancel any operations on this socket
        // The other end is likely waiting to read from it, so a cancel signals it is time to close
        boost::system::error_code cancel_error;
        connection.destination.cancel(cancel_error);

        connection.finished = true;
        on_finish();
    }

    void websocket_service::on_finish() {
        if (client_connection.finished && server_connection.finished) {
            interceptors.websocket.run(intercept::websocket_event::stop, flow, pline);
            stop();
        }
    }
}