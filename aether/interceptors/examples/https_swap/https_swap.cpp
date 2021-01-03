/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "https_swap.hpp"

namespace interceptors::examples {
    // Swap Facebook for Twitter and vice versa
    // This is not a perfect solution because browsers will put up a fight against
    // sharing cookies and some SSL certificates may not have the other domain on it
    // that the browser thinks it is getting, but it works well when browsing as a guest

    // Header that is added to intercepted requests so their responses can be checked
    static constexpr std::string_view marker = "aether-https-swap";
    static constexpr std::string_view facebook_site = "www.facebook.com";
    static constexpr std::string_view twitter_site = "twitter.com";

    void on_http_request(connection_flow &flow, http::exchange &exch) {
        http::request &req = exch.request();
        const http::url &target = req.get_target();

        // Switch target and Host header
        if (target.is_host(facebook_site)) {
            req.add_header(marker);
            req.update_host(twitter_site, target.get_port());
        }
        else if (target.is_host(twitter_site)) {
            req.add_header(marker);
            req.update_host(facebook_site, target.get_port());
        }
        // May need to switch Origin header for API resources
        else if (req.has_header("Origin")) {
            http::url origin = http::url::parse(req.get_header("Origin"));
            if (origin.is_host(facebook_site)) {
                origin.netloc.host = twitter_site;
                req.update_origin_and_referer(origin);
                req.add_header(marker);
            }
            else if (origin.is_host(twitter_site)) {
                origin.netloc.host = facebook_site;
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

            // For resources blocked behind a same-origin CORS policy
            if (res.has_header("Access-Control-Allow-Origin")) {
                http::url origin = http::url::parse(res.get_header("Access-Control-Allow-Origin"));
                if (origin.is_host(facebook_site)) {
                    origin.netloc.host = twitter_site;
                    res.set_header_to_value("Access-Control-Allow-Origin", origin.origin_string());
                }
                else if (origin.is_host(twitter_site)) {
                    origin.netloc.host = facebook_site;
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