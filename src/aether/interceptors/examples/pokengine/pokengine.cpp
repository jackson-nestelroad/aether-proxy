/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "pokengine.hpp"

#include "aether/util/console.hpp"

namespace interceptors::examples {

void pokengine_interceptor::on_http_connect(connection_flow& flow, http::exchange& exch) {
  if (exch.request().target().is_host(host_name, port)) {
    exch.set_mask_connect(true);
  }
}

void pokengine_interceptor::on_websocket_start(connection_flow& flow, websocket::pipeline& pline) {
  if (flow.server.is_connected_to(host_name, port)) {
    pline.set_interception(true);
    out::safe_console::log("Server connection started");
  }
}

void pokengine_interceptor::on_websocket_stop(connection_flow& flow, websocket::pipeline& pline) {
  if (flow.server.is_connected_to(host_name, port)) {
    out::safe_console::log("Server connection finished");
  }
}

void pokengine_interceptor::on_websocket_error(connection_flow& flow, websocket::pipeline& pline) {
  if (flow.server.is_connected_to(host_name, port)) {
    out::safe_error::log(flow.error);
  }
}

void pokengine_interceptor::on_websocket_message_received(connection_flow& flow, websocket::pipeline& pline,
                                                          websocket::message& msg) {
  if (flow.server.is_connected_to(host_name, port)) {
    out::safe_console::stream(msg.size(), " bytes received from the ", msg.origin(), '\n');
  }
}

}  // namespace interceptors::examples
