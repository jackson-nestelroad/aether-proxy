/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "websocket_service.hpp"

#include <functional>
#include <utility>

#include "aether/proxy/base_service.hpp"
#include "aether/proxy/connection/connection_flow.hpp"
#include "aether/proxy/connection_handler.hpp"
#include "aether/proxy/http/exchange.hpp"
#include "aether/proxy/server_components.hpp"
#include "aether/proxy/websocket/pipeline.hpp"
#include "aether/proxy/websocket/protocol/websocket_manager.hpp"
#include "aether/util/result_macros.hpp"

namespace proxy::websocket {
websocket_service::websocket_service(connection::connection_flow& flow, connection_handler& owner,
                                     server_components& components, http::exchange& handshake)
    : base_service(flow, owner, components),
      pipeline_(handshake, options_.websocket_intercept_messages_by_default),
      client_manager_(endpoint::client, pipeline_.extensions()),
      server_manager_(endpoint::server, pipeline_.extensions()),
      client_connection_{endpoint::server, endpoint::client, static_cast<connection::base_connection&>(flow.server),
                         static_cast<connection::base_connection&>(flow.client), client_manager_},
      server_connection_{endpoint::client, endpoint::server, static_cast<connection::base_connection&>(flow.client),
                         static_cast<connection::base_connection&>(flow.server), server_manager_} {}

void websocket_service::start() {
  interceptors_.websocket.run(intercept::websocket_event::start, flow_, pipeline_);

  // Run the two connection loops.
  // They start in the same thread but then diverge to operate asynchronously.
  websocket_loop(client_connection_);
  websocket_loop(server_connection_);
}

void websocket_service::websocket_loop(websocket_connection& connection) {
  if (result<void> res = websocket_loop_impl(connection); !res.is_ok()) {
    // Abnormal closure.
    flow_.error = std::move(res).err();
    pipeline_.set_close_state(connection.source_ep, {close_code::protocol_error, flow_.error.message()});
    interceptors_.websocket.run(intercept::websocket_event::error, flow_, pipeline_);
    close_connection(connection);
  }
}

result<void> websocket_service::websocket_loop_impl(websocket_connection& connection) {
  // Connection closed, and it closed from the other end.
  // Close the connection from this end.
  if (pipeline_.closed()) {
    close_connection(connection);
  } else {
    // Parse input buffer.
    std::optional<close_code> close;
    ASSIGN_OR_RETURN(std::vector<completed_frame> frames, connection.manager.parse(connection.source.input_buffer(), close));

    // Close connection if parser indicates.
    if (close.has_value()) {
      pipeline_.set_close_state(connection.source_ep, {close.value()});
      close_connection(connection);
    } else {
      // Handle frames accordingly.
      for (completed_frame& frame : frames) {
        switch (frame.type()) {
          case opcode::ping:
            RETURN_IF_ERROR(on_ping_frame(connection, std::move(frame).get_ping_frame()));
            break;
          case opcode::pong:
            on_pong_frame(connection, std::move(frame).get_pong_frame());
            break;
          case opcode::close:
            on_close_frame(connection, std::move(frame).get_close_frame());
            break;
          case opcode::binary:
          case opcode::text:
            RETURN_IF_ERROR(on_message_frame(connection, std::move(frame).get_message_frame()));
            break;
          default:
            break;
        }
      }

      // Connection was just closed by an event handler.
      if (pipeline_.closed()) {
        close_connection(connection);
        return util::ok;
      }

      // Inject frames.
      while (pipeline_.has_frame(connection.destination_ep)) {
        RETURN_IF_ERROR(connection.manager.serialize(connection.destination.output_buffer(),
                                                     std::move(pipeline_.next_frame(connection.destination_ep))));
        pipeline_.pop_frame(connection.destination_ep);
      }

      // Inject messages.
      while (pipeline_.has_message(connection.destination_ep)) {
        RETURN_IF_ERROR(send_message(connection, std::move(pipeline_.next_message(connection.destination_ep))));
        pipeline_.pop_message(connection.destination_ep);
      }

      connection.destination.write_untimed_async(
          [this, &connection](const boost::system::error_code& error, std::size_t bytes_transferred) {
            on_destination_write(error, bytes_transferred, connection);
          });
    }
  }
  return util::ok;
}

result<void> websocket_service::on_ping_frame(websocket_connection& connection, ping_frame&& frame) {
  // Inject a pong frame back to the source.
  pipeline_.inject_frame(connection.source_ep, frame.response());

  // Send a ping frame to the destination.
  return connection.manager.serialize(connection.destination.output_buffer(), std::move(frame));
}

void websocket_service::on_pong_frame(websocket_connection& connection, pong_frame&& frame) {
  // Do nothing.
}

void websocket_service::on_close_frame(websocket_connection& connection, close_frame&& frame) {
  // We only need to set the close state for the pipeline.
  // When both sides move to the next iteration of websocket_loop, the connection will see that the pipeline is closed
  // and send the appropriate close frame.
  pipeline_.set_close_state(connection.source_ep, frame);
}

result<void> websocket_service::on_message_frame(websocket_connection& connection, message_frame&& frame) {
  if (pipeline_.should_intercept()) {
    // Connection was set to intercept mid-way through a message, wait until the message finishes.
    if (!connection.ready_to_intercept) {
      if (frame.finished) {
        connection.ready_to_intercept = true;
      }
    } else {
      connection.next_message_content += std::move(frame.payload);

      if (frame.finished) {
        message msg{frame.type, connection.source_ep};
        msg.set_content(std::move(connection.next_message_content));
        connection.next_message_content.clear();

        // Message may be altered or blocked
        interceptors_.websocket_message.run(intercept::websocket_message_event::received, flow_, pipeline_, msg);

        RETURN_IF_ERROR(send_message(connection, std::move(msg)));
      }
    }
  } else {
    connection.ready_to_intercept = false;
    RETURN_IF_ERROR(connection.manager.serialize(connection.destination.output_buffer(), std::move(frame)));
  }
  return util::ok;
}

result<void> websocket_service::send_message(websocket_connection& connection, message&& msg) {
  if (!msg.blocked()) {
    std::string_view content = msg.content();
    std::size_t chunk_size = connection.source_ep == endpoint::client ? client_chunk_size : server_chunk_size;
    std::size_t content_length = content.length();
    std::size_t i = 0;
    bool finished = false;
    do {
      finished = i + chunk_size >= content_length;
      RETURN_IF_ERROR(connection.manager.serialize(
          connection.destination.output_buffer(),
          message_frame{msg.type(), finished, std::string(content.substr(i, chunk_size))}));
      i += chunk_size;
    } while (!finished);
  }
  return util::ok;
}

void websocket_service::on_destination_write(const boost::system::error_code& error, std::size_t bytes_transferred,
                                             websocket_connection& connection) {
  if (error != boost::system::errc::success) {
    on_error(error, connection);
  } else {
    connection.source.read_async([this, connection = std::ref(connection)](const boost::system::error_code& error,
                                                                           std::size_t bytes_transferred) {
      on_source_read(error, bytes_transferred, connection);
    });
  }
}

void websocket_service::on_source_read(const boost::system::error_code& error, std::size_t bytes_transferred,
                                       websocket_connection& connection) {
  // Error reading new data, close the connection
  if (error != boost::system::errc::success) {
    if (error == boost::asio::error::operation_aborted && pipeline_.closed()) {
      // Looks like cancel() was called and the pipeline is closed, so close the connection.
      close_connection(connection);
    } else {
      // Some other error occurred
      on_error(error, connection);
    }
  } else {
    websocket_loop(connection);
  }
}

void websocket_service::on_error(const boost::system::error_code& error, websocket_connection& connection) {
  if (!pipeline_.closed()) {
    pipeline_.set_close_state(connection.source_ep, {close_code::internal_error, error.message()});
  }
  flow_.error.set_boost_error(error);
  interceptors_.websocket.run(intercept::websocket_event::error, flow_, pipeline_);

  if (error == boost::asio::error::eof) {
    // Looks like the connection closed, no use trying to send a close frame.
    finish_connection(connection);
  } else {
    close_connection(connection);
  }
}

void websocket_service::close_connection(websocket_connection& connection) {
  if (!connection.finished) {
    if (result<void> res =
            connection.manager.serialize(connection.destination.output_buffer(), pipeline_.get_close_frame());
        !res.is_ok()) {
      flow_.error = std::move(res).err();
      interceptors_.websocket.run(intercept::websocket_event::error, flow_, pipeline_);

      // A serialization error when closing the connection cannot really be recovered from.
      finish_connection(connection);
    } else {
      connection.destination.write_untimed_async(
          [this, &connection](const boost::system::error_code& error, std::size_t bytes_transferred) {
            on_close(error, bytes_transferred, connection);
          });
    }
  } else {
    // Connection has already been properly closed, just go straight to finish handler.
    on_finish();
  }
}

void websocket_service::on_close(const boost::system::error_code& error, std::size_t bytes_transferred,
                                 websocket_connection& connection) {
  if (error != boost::system::errc::success) {
    flow_.error.set_boost_error(error);
    interceptors_.websocket.run(intercept::websocket_event::error, flow_, pipeline_);
  }
  finish_connection(connection);
}

void websocket_service::finish_connection(websocket_connection& connection) {
  // Cancel any operations on this socket.
  // The other end is likely waiting to read from it, so a cancel signals it is time to close.
  if (connection.destination.can_be_shutdown()) {
    connection.destination.shutdown();
  }
  connection.finished = true;
  on_finish();
}

void websocket_service::on_finish() {
  if (client_connection_.finished && server_connection_.finished) {
    interceptors_.websocket.run(intercept::websocket_event::stop, flow_, pipeline_);
    stop();
  }
}

}  // namespace proxy::websocket
