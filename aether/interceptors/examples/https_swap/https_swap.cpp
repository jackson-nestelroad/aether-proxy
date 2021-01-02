/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "https_swap.hpp"

namespace interceptors::examples {
    // Header that is added to intercepted requests so their responses can be checked
    static constexpr std::string_view marker = "aether-https-swap";

    void on_http_request(connection_flow &flow, http::exchange &exch) {
        http::request &req = exch.request();
        const http::url &target = req.get_target();

        // Switch target and Host header
        if (target.is_host("www.facebook.com")) {
            req.add_header(marker);
            req.update_host("twitter.com", target.get_port());
        }
        else if (target.is_host("twitter.com")) {
            req.add_header(marker);
            req.update_host("www.facebook.com", target.get_port());
        }
        // May need to switch Origin header for API resources
        else if (req.has_header("Origin")) {
            http::url origin = http::url::parse(req.get_header("Origin"));
            if (origin.is_host("www.facebook.com")) {
                origin.netloc.host = "twitter.com";
                req.update_origin_and_referer(origin);
                req.add_header(marker);
            }
            else if (origin.is_host("twitter.com")) {
                origin.netloc.host = "www.facebook.com";
                req.update_origin_and_referer(origin);
                req.add_header(marker);
            }
        }
    }

    void on_http_response(connection_flow &flow, http::exchange &exch) {
        http::request &req = exch.request();

        // This request was previously marked
        if (req.has_header("aether-https-swap")) {
            http::response &res = exch.response();
            const http::url &target = exch.request().get_target();

            // For resources blocked behind a same-origin CORS policy
            if (res.has_header("Access-Control-Allow-Origin")) {
                http::url origin = http::url::parse(res.get_header("Access-Control-Allow-Origin"));
                if (origin.is_host("www.facebook.com")) {
                    origin.netloc.host = "twitter.com";
                    res.set_header_to_value("Access-Control-Allow-Origin", origin.origin_string());
                }
                else if (origin.is_host("twitter.com")) {
                    origin.netloc.host = "www.facebook.com";
                    res.set_header_to_value("Access-Control-Allow-Origin", origin.origin_string());
                }
            }
        }
    }

    void attach_https_swap_example(proxy::server &server) {
        server.interceptors.http.attach(http_event::any_request, on_http_request);
        server.interceptors.http.attach(http_event::response, on_http_response);
    }
}