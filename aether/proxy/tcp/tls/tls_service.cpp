/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "tls_service.hpp"
#include <aether/proxy/connection_handler.hpp>

namespace proxy::tcp::tls {
    tls_service::tls_service(connection::connection_flow &flow, connection_handler &owner,
        tcp::intercept::interceptor_manager &interceptors)
        : base_service(flow, owner, interceptors)
    { }
    
    void tls_service::start() {

    }
}