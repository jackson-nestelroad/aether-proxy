/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "attach.hpp"

#include <aether/interceptors/http/http_logger.hpp>
#include <aether/interceptors/http/disable_h2c.hpp>

namespace interceptors {
    void attach_options(proxy::server &server, program::options &options) {
        // TODO: Command-line options for which interceptors to use
        attach_http_interceptor<http::logging_service<&std::cout>>(server);
        attach_http_interceptor<http::disable_h2c>(server);
    }
}