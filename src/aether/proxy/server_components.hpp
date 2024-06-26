/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <memory>

#include "aether/program/options.hpp"
#include "aether/proxy/concurrent/io_context_pool.hpp"
#include "aether/proxy/connection/connection_manager.hpp"
#include "aether/proxy/intercept/interceptor_services.hpp"
#include "aether/proxy/tls/x509/client_store.hpp"
#include "aether/proxy/tls/x509/server_store.hpp"
#include "aether/util/uuid.hpp"

namespace proxy {

// Class for owning all server-wide components.
class server_components {
 public:
  server_components() = delete;
  ~server_components() = default;
  server_components(const server_components& other) = delete;
  server_components& operator=(const server_components& other) = delete;
  server_components(server_components&& other) noexcept = delete;
  server_components& operator=(server_components&& other) noexcept = delete;

  inline tls::x509::client_store& client_store() { return *client_store_ptr; }
  inline const tls::x509::client_store& client_store() const { return *client_store_ptr; }

  inline tls::x509::server_store& server_store() { return *server_store_ptr; }
  inline const tls::x509::server_store& server_store() const { return *server_store_ptr; }

  program::options options;
  concurrent::io_context_pool io_contexts;
  intercept::interceptor_manager interceptors;
  util::uuid_factory uuid_factory;
  connection::connection_manager connection_manager;

 private:
  server_components(program::options options);

  std::unique_ptr<tls::x509::client_store> client_store_ptr;
  std::unique_ptr<tls::x509::server_store> server_store_ptr;

  friend class server;
  friend class server_builder;
};

}  // namespace proxy
