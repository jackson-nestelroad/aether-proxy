/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "teapot.hpp"

namespace interceptors::examples::teapot {
    // Insert a response at http://tea.pot/, which does not even belong to a server
    void give_teapot_response(connection_flow &flow, http::exchange &exch) {
        http::request &req = exch.request();
        if (req.get_method() == http::method::GET && exch.request().get_target().is_host("tea.pot")) {
            http::response &res = exch.make_response();
            res.set_status(http::status::im_a_teapot);
            res.add_header("Content-Type", "text/plain");
            res.set_body("I'm a little teapot, short and stout!");
            res.set_content_length();
        }
    }

    // Enable https://tea.pot/
    void allow_teapot_https(connection_flow &flow) {
        // An attempt was made to connect to a server, but no server exists!
        // Purposefully clear this error so the request is allowed to go through
        if (flow.server.get_host() == "tea.pot" && !flow.server.connected() && flow.error.get_proxy_error() == proxy::errc::upstream_connect_error) {
            flow.error.clear();
        }
    }

    // Intercept requests to www.google.com twith a teapot response
    void intercept_with_teapot(connection_flow &flow, http::exchange &exch) {
        http::request &req = exch.request();
        if (req.get_method() == http::method::GET && req.get_target().is_host("www.google.com")) {
            http::response &res = exch.make_response();
            res.set_status(http::status::im_a_teapot);
            res.add_header("Content-Type", "text/plain");
            res.set_body("I'm a little teapot, short and stout!");
            res.set_content_length();
        }
    }

    void attach_teapot_example(proxy::server &server) {
        // Enable http://tea.pot/ and https://tea.pot/
        server.interceptors.http.attach(http_event::request, give_teapot_response);
        server.interceptors.tls.attach(tls_event::established, allow_teapot_https);
        
        // GET Google ==> teapot
        server.interceptors.http.attach(http_event::request, intercept_with_teapot);
    }
}