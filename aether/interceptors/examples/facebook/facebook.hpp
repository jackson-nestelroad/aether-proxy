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
        An example for serving Facebook on a custom domain.
    */
    class facebook_interceptor : public interceptor_hub {
    private:
        http::cookie_collection cookies;

    public:
        // Header that is added to intercepted requests so their responses can be checked
        static constexpr std::string_view marker = "aether-facebook";
        static constexpr std::string_view facebook_site = "www.facebook.com";
        static constexpr std::string_view facebook = "facebook.com";
        static constexpr std::string_view spoofed_site = "my.face.book";
        static constexpr std::string_view redirect_to = "https://my.face.book/";

        void on_http_connect(connection_flow &flow, http::exchange &exch) override;
        void on_http_request(connection_flow &flow, http::exchange &exch) override;
        void on_http_response(connection_flow &flow, http::exchange &exch) override;
        void on_ssl_certificate_create(connection_flow &flow, tls::x509::certificate_interface &cert_interface) override;
    };
}
