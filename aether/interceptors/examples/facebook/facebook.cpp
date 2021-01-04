/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "facebook.hpp"

namespace interceptors::examples {
    void facebook_interceptor::on_http_connect(connection_flow &flow, http::exchange &exch) {
        http::request &req = exch.request();
        http::url target = req.get_target();

        // Starting an HTTPS connection, make sure it connects to the existing server
        if (target.is_host(spoofed_site)) {
            req.update_host(facebook_site, target.get_port());
        }
    }

    void facebook_interceptor::on_http_request(connection_flow &flow, http::exchange &exch) {
        http::request &req = exch.request();
        http::url target = req.get_target();

        // Redirect Facebook to our site
        // Pages that use an absolute link will have to go through the redirect first
        if (target.is_host(facebook_site)) {
            target.netloc.host = spoofed_site;
            if (target.scheme.empty()) {
                target.scheme = "https";
            }

            http::response &res = exch.make_response();
            res.set_status(http::status::found);
            res.set_header_to_value("Location", target.absolute_string());
            res.set_content_length();
        }
        else if (target.is_host(spoofed_site)) {
            // Force HTTPS on our website
            if (target.get_port() == 80 && !flow.client.secured()) {
                http::response &res = exch.make_response();
                res.set_status(http::status::moved_permanently);
                res.set_header_to_value("Location", redirect_to);
                res.set_content_length();
                return;
            }
            else {
                // Mark the request with a header so we can view the response later
                req.add_header(marker);

                // Direct request to Facebook
                target.netloc.host = facebook_site;

                // Facebook client checks the site domain it is being served on when fetching this resource
                if (target.path == "/intern/common/referer_frame.php") {
                    target.path = "/common/referer_frame.php";
                }

                req.update_target(target);

                // Facebook client also tries to remove the Cookie header when the site domain is off for a single critical request
                if (!req.has_header("Cookie") && !cookies.empty()) {
                    req.set_cookies(cookies);
                }
                // New proxy session, so we need the cookies for our domain
                else if (req.has_header("Cookie") && cookies.empty()) {
                    cookies.update(req.get_cookies());
                }
            }
        }

        // May need to switch Origin and Referer headers for API resources and server requests
        if (req.has_header("Origin")) {
            http::url origin = http::url::parse(req.get_header("Origin"));
            if (origin.is_host(spoofed_site)) {
                origin.netloc.host = facebook_site;
                req.update_origin_and_referer(origin);
                req.add_header(marker);
            }
        }
        else if (req.has_header("Referer")) {
            http::url origin = http::url::parse(req.get_header("Referer"));
            if (origin.is_host(spoofed_site)) {
                origin.netloc.host = facebook_site;
                req.update_origin_and_referer(origin);
                req.add_header(marker);
            }
        }
    }

    void facebook_interceptor::on_http_response(connection_flow &flow, http::exchange &exch) {
        http::request &req = exch.request();

        // This request was previously marked
        if (req.has_header(util::string::as_string(marker))) {
            http::response &res = exch.response();

            // For resources blocked behind a same-origin CORS policy
            if (res.has_header("Access-Control-Allow-Origin")) {
                http::url origin = http::url::parse(res.get_header("Access-Control-Allow-Origin"));
                if (origin.is_host(facebook_site)) {
                    origin.netloc.host = spoofed_site;
                    res.set_header_to_value("Access-Control-Allow-Origin", origin.origin_string());
                }
            }
            else {
                res.set_header_to_value("Access-Control-Allow-Origin", "*");
            }

            // Forward 302 redirects
            if (res.get_status() == http::status::found) {
                http::url redirect = http::url::parse(res.get_header("Location"));
                if (redirect.is_host(facebook_site)) {
                    redirect.netloc.host = spoofed_site;
                    res.set_header_to_value("Location", redirect.absolute_string());
                }
            }

            // Swap the domain for any set cookies so that our client can receive them
            if (res.has_cookies()) {
                http::cookie_collection set_cookies = res.get_cookies();

                for (auto &[name, cookie] : set_cookies) {
                    auto &&domain = cookie.domain();
                    if (domain.has_value()) {
                        if (domain.value() == facebook) {
                            cookie.set_domain(spoofed_site);
                        }
                    }
                }

                res.set_cookies(set_cookies);
                cookies.update(set_cookies);
            }
        }
    }

    void facebook_interceptor::on_ssl_certificate_create(connection_flow &flow, tls::x509::certificate_interface &cert_interface) {
        // Put the other endpoint in the SANS section of the new SSL certificate
        // Otherwise the browser may complain when the endpoints don't line up
        if (cert_interface.common_name.has_value()) {
            const std::string &common_name = cert_interface.common_name.value();
            if (util::string::ends_with(common_name, facebook)) {
                cert_interface.sans.emplace(spoofed_site);
            }
        }
    }
}