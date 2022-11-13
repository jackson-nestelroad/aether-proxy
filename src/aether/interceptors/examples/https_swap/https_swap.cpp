/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "https_swap.hpp"

namespace interceptors::examples::https_swap {

// Swap Facebook for Twitter and vice versa.
// This is not a perfect solution, as websites try to protect against things exactly like this, but this solution works
// somewhat well.

// There are some edge cases that likely need to be handled for both sites.
// The Facebook edge cases are handled in the facebook_interceptor object.

// However, if Facebook gives a link to itself (www.facebook.com) with an absolute link, the proxy will think it should
// go to Twitter

static constexpr std::string_view marker = "aether-https-swap";
static constexpr std::string_view facebook_site = "www.facebook.com";
static constexpr std::string_view facebook = "facebook.com";
static constexpr std::string_view twitter = "twitter.com";

void on_http_request(connection_flow& flow, http::exchange& exch) {
  http::request& req = exch.request();
  http::url target = req.target();

  // Switch target and Host header.
  if (target.is_host(facebook_site)) {
    req.add_header(marker);
    req.update_host(twitter, target.port_or_default(80));
  } else if (target.is_host(twitter)) {
    req.add_header(marker);
    target.netloc.host = facebook_site;
    req.update_target(target);
  }

  // May need to switch Origin and Referer headers for API resources and server requests.
  if (req.has_header("Origin")) {
    http::url origin = http::url::parse(req.get_header("Origin"));
    if (origin.is_host(facebook_site)) {
      origin.netloc.host = twitter;
      req.update_origin_and_referer(origin);
      req.add_header(marker);
    } else if (origin.is_host(twitter)) {
      origin.netloc.host = facebook_site;
      req.update_origin_and_referer(origin);
      req.add_header(marker);
    }
  } else if (req.has_header("Referer")) {
    http::url origin = http::url::parse(req.get_header("Referer"));
    if (origin.is_host(facebook_site)) {
      origin.netloc.host = twitter;
      req.update_origin_and_referer(origin);
      req.add_header(marker);
    } else if (origin.is_host(twitter)) {
      origin.netloc.host = facebook_site;
      req.update_origin_and_referer(origin);
      req.add_header(marker);
    }
  }
}

void on_http_response(connection_flow& flow, http::exchange& exch) {
  http::request& req = exch.request();

  // This request was previously marked.
  if (req.has_header(marker.data())) {
    http::response& res = exch.response();

    // For resources blocked behind a same-origin CORS policy.
    if (res.has_header("Access-Control-Allow-Origin")) {
      http::url origin = http::url::parse(res.get_header("Access-Control-Allow-Origin"));
      if (origin.is_host(facebook_site)) {
        origin.netloc.host = twitter;
        res.set_header_to_value("Access-Control-Allow-Origin", origin.origin_string());
      } else if (origin.is_host(twitter)) {
        origin.netloc.host = facebook_site;
        res.set_header_to_value("Access-Control-Allow-Origin", origin.origin_string());
      }
    } else {
      res.set_header_to_value("Access-Control-Allow-Origin", "*");
    }

    // Forward 302 redirects.
    if (res.status() == http::status::found) {
      http::url redirect = http::url::parse(res.get_header("Location"));
      if (redirect.is_host(facebook_site)) {
        redirect.netloc.host = twitter;
        res.set_header_to_value("Location", redirect.absolute_string());
      } else if (redirect.is_host(twitter)) {
        redirect.netloc.host = facebook_site;
        res.set_header_to_value("Location", redirect.absolute_string());
      }
    }

    // Swap the domain for any set cookies.
    if (res.has_cookies()) {
      http::cookie_collection cookies = res.get_cookies();
      for (auto& [name, cookie] : cookies) {
        auto domain = cookie.domain();
        if (domain.has_value()) {
          std::string_view domain_value = domain.value();
          if (domain_value == facebook) {
            cookie.set_attribute("Domain", twitter);
          } else if (domain_value == twitter) {
            cookie.set_attribute("Domain", facebook);
          }
        }
        res.set_cookies(cookies);
      }
    }
  }
}

void on_ssl_certificate_create(connection_flow& flow, tls::x509::certificate_interface& cert_interface) {
  // Put the other endpoint in the SANS section of the new SSL certificate.
  // Otherwise the browser may complain when the endpoints don't line up.
  if (cert_interface.common_name.has_value()) {
    const std::string& common_name = cert_interface.common_name.value();
    if (util::string::ends_with(common_name, facebook)) {
      cert_interface.sans.emplace(twitter);
    } else if (util::string::ends_with(common_name, twitter)) {
      cert_interface.sans.emplace(facebook_site);
    }
  }
}

void attach_https_swap_example(proxy::server& server) {
  server.interceptors().http.attach(http_event::any_request, on_http_request);
  server.interceptors().http.attach(http_event::response, on_http_response);
  server.interceptors().ssl_certificate.attach(ssl_certificate_event::create, on_ssl_certificate_create);
}

}  // namespace interceptors::examples::https_swap
