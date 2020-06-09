/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "attach.hpp"

#include <aether/interceptors/http/http_logger.hpp>

namespace interceptors {
    void attach_options(proxy::server &server, program::options &options) {
        // TODO: Command-line options for which interceptors to use
        attach_http_interceptor<http::logging_service<&std::cout>>(server);
    }
}