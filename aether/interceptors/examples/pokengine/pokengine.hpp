/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/server.hpp>

using namespace proxy::tcp;
using namespace proxy::tcp::intercept;
using namespace proxy::connection;

namespace interceptors::examples {
    /*
        An example interceptor hub for watching data when interacting with the Pokengine game server.
    */
    class pokengine_interceptor : public interceptor_hub {
    public:
        static constexpr std::string_view host_name = "pokengine.org";
        static constexpr proxy::port_t port = 9875;

        void on_http_connect(connection_flow &flow, http::exchange &exch) override;
        void on_websocket_start(connection_flow &flow, websocket::pipeline &pline) override;
        void on_websocket_stop(connection_flow &flow, websocket::pipeline &pline) override;
        void on_websocket_error(connection_flow &flow, websocket::pipeline &pline) override;
        void on_websocket_message_received(connection_flow &flow, websocket::pipeline &pline, websocket::message &msg) override;
    };
}