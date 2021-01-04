/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "events.hpp"

namespace interceptors::examples::events {
    void on_server_connect(connection_flow &flow) {
        out::safe_console::stream("Connected to ", flow.server.get_host(), " (", flow.server.get_endpoint(), ")\n");
    }

    void on_server_disconnect(connection_flow &flow) {
        out::safe_console::stream("Disconected from ", flow.server.get_host(), " (", flow.server.get_endpoint(), ")\n");
    }

    void on_http_request(connection_flow &flow, http::exchange &exch) {
        http::request &req = exch.request();
        out::safe_console::stream(req.get_method(), " request to ", req.get_target().absolute_string(), '\n');
    }

    void on_http_connect(connection_flow &flow, http::exchange &exch) {
        http::request &req = exch.request();
        out::safe_console::stream("CONNECT request to ", req.get_target().absolute_string(), '\n');
    }

    void on_http_any_request(connection_flow &flow, http::exchange &exch) {
        http::request &req = exch.request();
        out::safe_console::stream(req.get_method(), " request to ", req.get_target().absolute_string(), '\n');
    }

    void on_http_websocket_handshake(connection_flow &flow, http::exchange &exch) {
        http::request &req = exch.request();
        out::safe_console::stream("WebSocket handshake request to ", req.get_target().absolute_string(), '\n');
    }

    void on_http_response(connection_flow &flow, http::exchange &exch) {
        http::response &res = exch.response();
        out::safe_console::stream(res.get_status(), " response from ", exch.request().get_target().absolute_string(), '\n');
    }

    void on_http_error(connection_flow &flow, http::exchange &exch) {
        out::safe_error::stream("HTTP error: ", flow.error, '\n');
    }

    void on_tunnel_start(connection_flow &flow) {
        out::safe_console::stream("TCP tunnel initiated with ", flow.server.get_host(), " (", flow.server.get_endpoint(), ")\n");
    }

    void on_tunnel_stop(connection_flow &flow) {
        out::safe_console::stream("TCP tunnel finished with ", flow.server.get_host(), " (", flow.server.get_endpoint(), ")\n");
    }

    void on_tls_established(connection_flow &flow) {
        out::safe_console::stream("TLS established with ", flow.server.get_host(), " (", flow.server.get_endpoint(), ")\n");
    }

    void on_tls_error(connection_flow &flow) {
        out::safe_error::stream("TLS error: ", flow.error, '\n');
    }

    void on_ssl_certificate_search(connection_flow &flow, tls::x509::certificate_interface &cert_interface) {
        out::safe_console::stream("Searching for SSL certificate for ", cert_interface.common_name.value_or("[no CN]"));
    }

    void on_ssl_certificate_create(connection_flow &flow, tls::x509::certificate_interface &cert_interface) {
        out::safe_console::stream("Creating SSL certificate for ", cert_interface.common_name.value_or("[no CN]"));
    }

    void on_websocket_start(connection_flow &flow, websocket::pipeline &pline) {
        out::safe_console::stream("WebSocket connection established with ", flow.server.get_host(), " (", flow.server.get_endpoint(), ")\n");
    }

    void on_websocket_stop(connection_flow &flow, websocket::pipeline &pline) {
        out::safe_console::stream("WebSocket connection finished with ", flow.server.get_host(), " (", flow.server.get_endpoint(), ") (Close code = ", pline.get_close_code(), ")\n");
    }

    void on_websocket_error(connection_flow &flow, websocket::pipeline &pline) {
        out::safe_error::stream("WebSocket error: ", flow.error, '\n');
    }

    void on_websocket_message_received(connection_flow &flow, websocket::pipeline &pline, websocket::message &msg) {
        out::safe_console::stream("WebSocket message received from ", msg.get_origin(), '\n');
    }

    void attach_events(proxy::server &server) {
        server.interceptors.server.attach(intercept::server_event::connect, on_server_connect);
        server.interceptors.server.attach(intercept::server_event::disconnect, on_server_disconnect);

        server.interceptors.http.attach(intercept::http_event::request, on_http_request);
        server.interceptors.http.attach(intercept::http_event::connect, on_http_connect);
        server.interceptors.http.attach(intercept::http_event::any_request, on_http_any_request);
        server.interceptors.http.attach(intercept::http_event::websocket_handshake, on_http_websocket_handshake);
        server.interceptors.http.attach(intercept::http_event::response, on_http_response);
        server.interceptors.http.attach(intercept::http_event::error, on_http_error);

        server.interceptors.tunnel.attach(intercept::tunnel_event::start, on_tunnel_start);
        server.interceptors.tunnel.attach(intercept::tunnel_event::stop, on_tunnel_stop);

        server.interceptors.tls.attach(intercept::tls_event::established, on_tls_established);
        server.interceptors.tls.attach(intercept::tls_event::error, on_tls_error);

        server.interceptors.ssl_certificate.attach(intercept::ssl_certificate_event::search, on_ssl_certificate_search);
        server.interceptors.ssl_certificate.attach(intercept::ssl_certificate_event::create, on_ssl_certificate_create);

        server.interceptors.websocket.attach(intercept::websocket_event::start, on_websocket_start);
        server.interceptors.websocket.attach(intercept::websocket_event::stop, on_websocket_stop);
        server.interceptors.websocket.attach(intercept::websocket_event::error, on_websocket_error);
        server.interceptors.websocket_message.attach(intercept::websocket_message_event::received, on_websocket_message_received);
    }
}