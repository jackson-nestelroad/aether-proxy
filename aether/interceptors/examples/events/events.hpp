/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/server.hpp>

using namespace proxy::tcp;
using namespace proxy::connection;

namespace interceptors::examples {
    namespace events {
        /*
            Fires when a connection to a server has been established.
        */
        void on_server_connect(connection_flow &flow);

        /*
            Fires when a connection to a server has been disconnected.
        */
        void on_server_disconnect(connection_flow &flow);


        /*
            Fires when an HTTP request is received.
            Does not fire for CONNECT requests.
        */
        void on_http_request(connection_flow &flow, http::exchange &exch);

        /*
            Fires when an HTTP CONNECT request is received.
        */
        void on_http_connect(connection_flow &flow, http::exchange &exch);

        /*
            Fires when any HTTP request is received.
            Fires for CONNECT requests.
        */
        void on_http_any_request(connection_flow &flow, http::exchange &exch);
        
        /*
            Fires when a WebSocket handshake request is received.
        */
        void on_http_websocket_handshake(connection_flow &flow, http::exchange &exch);

        /*
            Fires when an HTTP response is received and being prepared
                to send to the client.
        */
        void on_http_response(connection_flow &flow, http::exchange &exch);

        /*
            Fires when an error occurs when handling an HTTP connection.
        */
        void on_http_error(connection_flow &flow, http::exchange &exch);

        
        /*
            Fires when a TCP tunnel is initiated on the connection.
        */
        void on_tunnel_start(connection_flow &flow);

        /*
            Fires when the connection's TCP tunnel finishes.
        */
        void on_tunnel_stop(connection_flow &flow);


        /*
            Fires when TLS has been successfully established across the connection.
        */
        void on_tls_established(connection_flow &flow);
        
        /*
            Fires when an error occurs when establishing TLS across the connection.
        */
        void on_tls_error(connection_flow &flow);
        

        /*
            Fires when an SSL certificate is going to be searched for and possibly generated.
        */
        void on_ssl_certificate_search(connection_flow &flow, tls::x509::certificate_interface &cert_interface);

        /*
            Fires when an SSL certificate is going to be created.
        */
        void on_ssl_certificate_search(connection_flow &flow, tls::x509::certificate_interface &cert_interface);


        /*
            Fires when a WebSocket connection is initiated.
        */
        void on_websocket_start(connection_flow &flow, websocket::pipeline &pline);

        /*
            Fires when a WebSocket connection finishes.
        */
        void on_websocket_stop(connection_flow &flow, websocket::pipeline &pline);

        /*
            Fires when an error occurs in a WebSocket connection.
        */
        void on_websocket_error(connection_flow &flow, websocket::pipeline &pline);

        /*
            Fires when a full WebSocket message (one or more frames) has been received.
        */
        void on_websocket_message_received(connection_flow &flow, websocket::pipeline &pline, websocket::message &msg);


        /*
            Function to attach interceptors to the server.
        */
        void attach_events(proxy::server &server);
    }
}