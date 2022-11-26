/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "server_components.hpp"

#include <memory>

namespace proxy {

server_components::server_components(program::options options)
    : options(std::move(options)), io_contexts(options.thread_pool_size), interceptors(), connection_manager(*this) {
  if (!options.ssl_passthrough_strict) {
    client_store_ptr = std::make_unique<tls::x509::client_store>(*this);
    server_store_ptr = std::make_unique<tls::x509::server_store>(*this);
  }
}

}  // namespace proxy
