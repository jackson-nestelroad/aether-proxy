/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "http_service.hpp"
#include <aether/proxy/connection_handler.hpp>

namespace proxy::tcp::http::http1 {
    const response http_service::continue_response = { version::http1_1, status::continue_, { }, "" };
    const response http_service::connect_response = { version::http1_1, status::ok, { }, "" };

    http_service::http_service(connection::connection_flow::ptr flow, connection_handler &owner)
        : base_service(flow, owner),
        exch(),
        _parser(exch)
    { }

    void http_service::start() {
        // Client connection has the initial HTTP request
        // We read the request headers and then handle it accordingly
        read_request_head();
    }

    void http_service::read_request_head() {
        // Read until the end of headers delimiter is found
        flow->client.read_until_async(message::CRLF_CRLF, boost::bind(&http_service::on_read_request_head, this,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void http_service::on_read_request_head(const boost::system::error_code &error, std::size_t bytes_transferred) {
        if (error != boost::system::errc::success) {
            if (error == boost::asio::error::operation_aborted) {
                send_error_response(status::request_timeout, error.message());
            }
            else {
                send_error_response(status::bad_request, error.message());
            }
        }
        else {
            try {
                std::istream input = flow->client.input_stream();
                _parser.read_request_line(input);
                _parser.read_headers(input, parser::message_mode::request);
                read_request_body(boost::bind(&http_service::handle_request, this));
            }
            catch (const error::base_exception &ex) {
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
            std::istream input = flow->client.input_stream();
            parser::body_parsing_status bp_status = _parser.read_body(input, parser::message_mode::request);
            
            // Need more data from the socket
            if (!bp_status.finished) {
                // Try to read rest of body
                if (bp_status.remaining != 0) {
                    flow->client.read_async(bp_status.remaining,
                        boost::bind(&http_service::on_read_request_body, this, handler,
                            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                }
                else {
                    flow->client.read_async(boost::bind(&http_service::on_read_request_body, this, handler,
                        boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                }
            }
            // Body is finished, call finished handler
            else {
                handler();
            }
        }
        catch (const error::base_exception &ex) {
            send_error_response(status::bad_request, ex.what());
        }
    }

    void http_service::on_read_request_body(const callback &handler, const boost::system::error_code &error, std::size_t bytes_transferred) {
        if (error != boost::system::errc::success) {
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
        request &req = exch.get_request();

        validate_target();
        // TODO: Interceptors here - give exchange and flow and allow them to change

        // TODO: Logging interceptor/service
        out::console::log(req.absolute_request_line_string());

        // This is a CONNECT request
        if (req.get_target().form == url::target_form::authority) {
            flow->server.set_host(req.get_host_name(), req.get_host_port());
            send_connect_response();
            return;
        }

        // TODO: Move this to a request interceptor
        // Disable HTTP/2.0, for now
        if (req.header_has_value("Upgrade", "h2c")) {
            req.remove_header("Upgrade");
        }

        if (req.header_has_value("Expect", "100-continue")) {
            flow->client << continue_response;
            // Write small response synchronusly
            boost::system::error_code write_err;
            flow->client.write(write_err);
            if (write_err != boost::system::errc::success) {
                stop();
                return;
            }
            req.remove_header("Expect");
        }

        // Response set by an interceptor
        if (exch.has_response()) {
            forward_response();
        }
        // Must get the response from the server
        else {
            connect_server();
        }
    }

    void http_service::validate_target() {
        request &req = exch.get_request();
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

        // Set default port
        if (!target.netloc.has_port()) {
            // TODO: Also check if connections are marked as secure?
            // Scheme may not be set...
            target.netloc.port = target.scheme == "https" ? 443 : 80;
        }

        req.set_target(target);
    }

    void http_service::connect_server() {
        auto target = exch.get_request().get_target();
        auto host = target.netloc.host;
        auto port = target.netloc.port.value();
        // Check if we are already connected to the server
        // Server may have also been closed, so we may need to reconnect
        if (flow->server.is_connected_to(host, port) && !flow->server.has_been_closed()) {
            forward_request();
        }
        else {
            flow->set_server(host, port);
            flow->connect_server_async(boost::bind(&http_service::on_connect_server, this,
                    boost::asio::placeholders::error));
        }
    }

    void http_service::on_connect_server(const boost::system::error_code &error) {
        if (error != boost::system::errc::success) {
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
        flow->server << exch.get_request();
        flow->server.write_async(boost::bind(&http_service::on_forward_request, this,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void http_service::on_forward_request(const boost::system::error_code &error, std::size_t bytes_transferred) {
        if (error != boost::system::errc::success) {
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
        flow->server.read_until_async(message::CRLF_CRLF, boost::bind(&http_service::on_read_response_head, this,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void http_service::on_read_response_head(const boost::system::error_code &error, std::size_t bytes_transferred) {
        if (error != boost::system::errc::success) {
            if (error == boost::asio::error::operation_aborted) {
                send_error_response(status::gateway_timeout, error.message());
            }
            else {
                send_error_response(status::internal_server_error, error.message());
            }
        }
        else {
            std::istream input = flow->server.input_stream();
            exch.make_response();
            _parser.read_response_line(input);
            _parser.read_headers(input, parser::message_mode::response);
            read_response_body(boost::bind(&http_service::forward_response, this));
        }
    }

    void http_service::read_response_body(const callback &handler) {
        // Just like read_request_body
        try {
            // Parse what we have, or what we just read
            std::istream input = flow->server.input_stream();
            parser::body_parsing_status bp_status = _parser.read_body(input, parser::message_mode::response);

            // Need more data from the socket
            if (!bp_status.finished) {
                // Try to read rest of body
                if (bp_status.remaining != 0) {
                    flow->server.read_async(bp_status.remaining,
                        boost::bind(&http_service::on_read_response_body, this, handler,
                            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                }
                else {
                    flow->server.read_async(boost::bind(&http_service::on_read_response_body, this, handler,
                        boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                }
            }
            // Body is finished, call finished handler
            else {
                handler();
            }
        }
        catch (const error::base_exception & ex) {
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
            else if (error == boost::asio::error::operation_aborted) {
                send_error_response(status::gateway_timeout, error.message());
            }
            else {
                send_error_response(status::bad_request, error.message());
            }
        }
        else {
            read_response_body(handler);
        }
    }

    void http_service::forward_response() {
        flow->client << exch.get_response();
        flow->client.write_async(boost::bind(&http_service::on_forward_response, this,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }
    void http_service::on_forward_response(const boost::system::error_code &error, std::size_t bytes_transferred) {
        if (error != boost::system::errc::success) {
            stop();
        }
        else {
            handle_response();
        }
    }

    void http_service::handle_response() {
        bool should_close = exch.get_request().should_close_connection() || exch.get_response().should_close_connection();
        if (should_close) {
            stop();
            return;
        }

        if (exch.get_response().get_status() == status::switching_protocols) {
            bool is_websocket = websocket::is_handshake(exch.get_request()) && websocket::is_handshake(exch.get_response());
            owner.switch_service<tunnel::tunnel_service>();
            return;
        }
        owner.switch_service<http_service>();
    }

    void http_service::send_connect_response() {
        flow->client << connect_response;
        flow->client.write_async(boost::bind(&http_service::on_send_connect_response, this,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void http_service::on_send_connect_response(const boost::system::error_code &error, std::size_t bytes_transferred) {
        owner.switch_service<tunnel::tunnel_service>();
    }

    void http_service::send_error_response(status response_status, std::string_view msg) {
        response &res = exch.make_response();
        res.set_status(response_status);

        std::string_view reason = status_to_reason(response_status);

        // A small hint of server-side rendering
        std::ostream content = res.content_stream();
        content << "<html><head>";
        content << "<title>" << response_status << ' ' << reason << "</title>";
        content << "</head><body>";
        content << "<h1>" << response_status << ' ' << reason << "</h1>";
        content << "<p>" << msg << "</p>";
        content << "</body></html>";

        res.add_header("Server", config::full_server_name.data());
        res.add_header("Connection", "close");
        res.add_header("Content-Type", "text/html");
        res.set_content_length();

        flow->client << res;
        flow->client.write_async(boost::bind(&http_service::on_write_error_response, this,
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void http_service::on_write_error_response(const boost::system::error_code &err, std::size_t bytes_transferred) {
        // No reason to check errors here
        stop();
    }

    exchange http_service::get_exchange() const {
        return exch;
    }
}