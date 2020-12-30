/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "http_service.hpp"
#include <aether/proxy/connection_handler.hpp>

namespace proxy::tcp::http::http1 {
    const response http_service::continue_response = { version::http1_1, status::continue_, { }, "" };
    const response http_service::connect_response = { version::http1_1, status::ok, { }, "" };

    http_service::http_service(connection::connection_flow &flow, connection_handler &owner,
        tcp::intercept::interceptor_manager &interceptors)
        : base_service(flow, owner, interceptors),
        exch(),
        parser(exch)
    { }

    void http_service::start() {
        // Errors may be stored up to be sent over HTTP
        if (flow.error.has_proxy_error()) {
            send_error_response(status::bad_gateway, flow.error.get_message_or_proxy());
        }
        else {
            // Client connection has the initial HTTP request
            // We read the request headers and then handle it accordingly
            read_request_head();
        }
    }

    void http_service::read_request_head() {
        // Read until the end of headers delimiter is found
        flow.client.read_until_async(message::CRLF_CRLF, boost::bind(&http_service::on_read_request_head, this,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void http_service::on_read_request_head(const boost::system::error_code &error, std::size_t bytes_transferred) {
        if (error != boost::system::errc::success) {
            // No new request started
            if (bytes_transferred == 0) {
                stop();
                return;
            }

            flow.error.set_boost_error(error);
            if (error == boost::asio::error::operation_aborted) {
                send_error_response(status::request_timeout, error.message());
            }
            else {
                send_error_response(status::bad_request, error.message());
            }
        }
        else {
            try {
                std::istream input = flow.client.input_stream();
                parser.read_request_line(input);
                parser.read_headers(input, http_parser::message_mode::request);
                read_request_body(boost::bind(&http_service::handle_request, this));
            }
            catch (const error::base_exception &ex) {
                flow.error.set_proxy_error(ex);
                send_error_response(status::bad_request, ex.what());
            }
        }
    }

    void http_service::read_request_body(const callback &handler) {
        // Unlike the headers, finding the end of the body is a bit tricky
        // The client's input buffer may not have all of the body in it
        // The flow for reading a HTTP body is like this:
            // 1. Figure out how to read the body (Content-Length, chunked encoding, or none at all)
            // 2. Read/parse whatever is in the input buffer
            // 3. Store body parsing state internally
            // 4. If the body is not complete, read from the socket and return to step 2.
            // 5. Body is complete, call handler.
        try {
            // Parse what we have, or what we just read
            std::istream input = flow.client.input_stream();

            // Need more data from the socket
            if (!parser.read_body(input, http_parser::message_mode::request)) {
                flow.client.read_async(boost::bind(&http_service::on_read_request_body, this, handler,
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            }
            // Body is finished, call finished handler
            else {
                handler();
            }
        }
        catch (const error::base_exception &ex) {
            flow.error.set_proxy_error(ex);
            send_error_response(status::bad_request, ex.what());
        }
    }

    void http_service::on_read_request_body(const callback &handler, const boost::system::error_code &error, std::size_t bytes_transferred) {
        if (error != boost::system::errc::success) {
            flow.error.set_boost_error(error);
            if (error == boost::asio::error::operation_aborted) {
                send_error_response(status::request_timeout, error.message());
            }
            else {
                send_error_response(status::bad_request, error.message());
            }
        }
        else {
            // Just go back to reading/parsing the body
            read_request_body(handler);
        }
    }

    void http_service::handle_request() {
        try {
            request &req = exch.request();

            validate_target();

            interceptors.http.run(intercept::http_event::any_request, flow, exch);

            // Insert Via header
            req.add_header("Via", out::string::stream("1.1 ", constants::lowercase_name));

            // This is a CONNECT request
            if (req.get_target().form == url::target_form::authority) {
                interceptors.http.run(intercept::http_event::connect, flow, exch);

                set_server(req.get_host_name(), req.get_host_port());

                // An interceptor may set a response
                // If it does not, use the default 200 response
                if (!exch.has_response()) {
                    exch.set_response(connect_response);
                }

                send_connect_response();
                return;
            }

            if (req.header_has_value("Expect", "100-continue")) {
                flow.client << continue_response;
                // Write small response synchronusly
                boost::system::error_code write_err;
                flow.client.write(write_err);
                if (write_err != boost::system::errc::success) {
                    stop();
                    return;
                }
                req.remove_header("Expect");
            }

            interceptors.http.run(intercept::http_event::request, flow, exch);
            set_server(req.get_host_name(), req.get_host_port());

            // Response set by an interceptor
            if (exch.has_response()) {
                forward_response();
            }
            // Must get the response from the server
            else {
                if (websocket::handshake::is_handshake(req)) {
                    interceptors.http.run(intercept::http_event::websocket_handshake, flow, exch);
                }
                connect_server();
            }
        }
        catch (const error::base_exception &ex) {
            flow.error.set_proxy_error(ex);
            send_error_response(status::bad_request, ex.what());
        }
    }

    void http_service::validate_target() {
        request &req = exch.request();
        url target = req.get_target();

        // Make sure target URL and host header are OK to be forwarded
        // This form is when the client knows it is talking to a proxy
        if (target.form == url::target_form::absolute) {
            // Add missing host header
            if (!req.has_header("Host")) {
                req.set_header_to_value("Host", target.netloc.to_host_string());
            }

            // Convert absolute form to origin form to forward the request
            // This is not technically required, but it's safer in case the server is expecting only an origin request
            target.form = url::target_form::origin;
        }
        // This form occurs when the client does not know it is talking to the proxy, so it sends an origin-form request
        // No host information in target, so we need to parse it and set it to keep things uniform
        else if (target.form == url::target_form::origin && !target.netloc.has_hostname()) {
            // Absolutely no way to get the intended host
            if (!req.has_header("Host")) {
                send_error_response(status::bad_request, "No host given.");
                return;
            }
            // Parse host header, and set the host
            target.netloc = url::parse_netloc(req.get_header("Host"));
        }

        // Set scheme based on server connection
        if (target.form != url::target_form::authority && target.scheme.empty()) {
            target.scheme = flow.server.secured() ? "https" : "http";
        }

        // Set default port
        if (!target.netloc.has_port()) {
            target.netloc.port = (target.scheme == "https" || flow.server.secured()) ? 443 : 80;
        }

        req.set_target(target);
    }

    void http_service::connect_server() {
        connect_server_async(boost::bind(&http_service::on_connect_server, this,
            boost::asio::placeholders::error));
    }

    void http_service::on_connect_server(const boost::system::error_code &error) {
        if (error != boost::system::errc::success) {
            flow.error.set_boost_error(error);
            if (error == boost::asio::error::operation_aborted) {
                send_error_response(status::gateway_timeout, error.message());
            }
            else {
                send_error_response(status::bad_gateway, error.message());
            }
        }
        else {
            forward_request();
        }
    }

    void http_service::forward_request() {
        flow.server << exch.request();
        flow.server.write_async(boost::bind(&http_service::on_forward_request, this,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void http_service::on_forward_request(const boost::system::error_code &error, std::size_t bytes_transferred) {
        if (error != boost::system::errc::success) {
            flow.error.set_boost_error(error);
            if (error == boost::asio::error::operation_aborted) {
                send_error_response(status::gateway_timeout, error.message());
            }
            else {
                send_error_response(status::internal_server_error, error.message());
            }
        }
        else {
            read_response_head();
        }
    }

    void http_service::read_response_head() {
        // Read until the end of headers delimiter is found
        flow.server.read_until_async(message::CRLF_CRLF, boost::bind(&http_service::on_read_response_head, this,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void http_service::on_read_response_head(const boost::system::error_code &error, std::size_t bytes_transferred) {
        if (error != boost::system::errc::success) {
            flow.error.set_boost_error(error);
            if (error == boost::asio::error::operation_aborted) {
                send_error_response(status::gateway_timeout, error.message());
            }
            else {
                send_error_response(status::internal_server_error, error.message());
            }
        }
        else {
            std::istream input = flow.server.input_stream();
            exch.make_response();
            parser.read_response_line(input);
            parser.read_headers(input, http_parser::message_mode::response);
            read_response_body(boost::bind(&http_service::forward_response, this));
        }
    }

    void http_service::read_response_body(const callback &handler) {
        // Just like read_request_body
        try {
            // Parse what we have, or what we just read
            std::istream input = flow.server.input_stream();

            // Need more data from the socket
            if (!parser.read_body(input, http_parser::message_mode::response)) {
                flow.server.read_async(boost::bind(&http_service::on_read_response_body, this, handler,
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            }
            // Body is finished, call finished handler
            else {
                handler();
            }
        }
        catch (const error::base_exception &ex) {
            flow.error.set_proxy_error(ex);
            send_error_response(status::bad_request, ex.what());
        }
    }

    void http_service::on_read_response_body(const callback &handler, const boost::system::error_code &error, std::size_t bytes_transferred) {
        if (error != boost::system::errc::success) {
            // Connection was closed by the server
            // This may be desired if reading body until end of stream
            if (error == boost::asio::error::eof) {
                read_response_body(handler);
            }
            else {
                flow.error.set_boost_error(error);
                if (error == boost::asio::error::operation_aborted) {
                    send_error_response(status::gateway_timeout, error.message());
                }
                else {
                    send_error_response(status::bad_request, error.message());
                }
            }
        }
        else {
            read_response_body(handler);
        }
    }

    void http_service::forward_response() {
        interceptors.http.run(intercept::http_event::response, flow, exch);

        flow.client << exch.response();
        flow.client.write_async(boost::bind(&http_service::on_forward_response, this,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }
    void http_service::on_forward_response(const boost::system::error_code &error, std::size_t bytes_transferred) {
        if (error != boost::system::errc::success) {
            flow.error.set_boost_error(error);
            stop();
        }
        else {
            handle_response();
        }
    }

    void http_service::handle_response() {
        bool should_close = exch.request().should_close_connection() || exch.response().should_close_connection();
        if (should_close) {
            stop();
            return;
        }

        if (exch.response().get_status() == status::switching_protocols) {
            if (!program::options::instance().websocket_passthrough_strict
                && (!program::options::instance().websocket_passthrough || flow.should_intercept_websocket())
                && websocket::handshake::is_handshake(exch.request()) && websocket::handshake::is_handshake(exch.response())) {
                owner.switch_service<websocket::websocket_service>(exch);
            }
            else {
                owner.switch_service<tunnel::tunnel_service>();
            }
        }
        else {
            owner.switch_service<http_service>();
        }
    }

    void http_service::send_connect_response() {
        flow.client << exch.response();
        flow.client.write_async(boost::bind(&http_service::on_send_connect_response, this,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void http_service::on_send_connect_response(const boost::system::error_code &error, std::size_t bytes_transferred) {
        if (exch.response().is_2xx()) {
            // Interceptors can mask a CONNECT request and treat it like a normal request
            if (exch.mask_connect()) {
                owner.switch_service<http_service>();
            }
            // Strict passthrough mode, use tunnel by default
            // Passthrough mode, use tunnel if not marked by a CONNECT interceptor
            else if (program::options::instance().ssl_passthrough_strict 
                || (program::options::instance().ssl_passthrough && !flow.should_intercept_tls())) {
                owner.switch_service<tunnel::tunnel_service>();
            }
            // Default, use TLS service
            else {
                // TLS may not be the correct option
                // If it is not, the stream will be forwarded to the TCP tunnel service
                owner.switch_service<tls::tls_service>();
            }
        }
        else {
            stop();
        }
    }

    void http_service::send_error_response(status response_status, std::string_view msg) {
        response &res = exch.make_response();
        res.set_status(response_status);

        std::string_view reason = convert::status_to_reason(response_status);

        // A small hint of server-side rendering
        std::stringstream content;
        content << "<html><head>";
        content << "<title>" << response_status << ' ' << reason << "</title>";
        content << "</head><body>";
        content << "<h1>" << response_status << ' ' << reason << "</h1>";
        content << "<p>" << msg << "</p>";
        content << "</body></html>";
        res.set_body(content.str());

        res.add_header("Server", constants::full_server_name.data());
        res.add_header("Connection", "close");
        res.add_header("Content-Type", "text/html");
        res.set_content_length();

        flow.client << res;
        flow.client.write_async(boost::bind(&http_service::on_write_error_response, this,
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void http_service::on_write_error_response(const boost::system::error_code &err, std::size_t bytes_transferred) {
        // No reason to check errors here
        interceptors.http.run(intercept::http_event::error, flow, exch);
        stop();
    }

    exchange http_service::get_exchange() const {
        return exch;
    }
}