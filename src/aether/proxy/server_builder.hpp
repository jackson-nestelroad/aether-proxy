/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include "aether/program/options_factory.hpp"
#include "aether/proxy/server.hpp"

namespace proxy {
class server;

class server_builder {
 public:
  server_builder() = default;
  ~server_builder() = default;
  server_builder(const server_builder& other) = default;
  server_builder& operator=(const server_builder& other) = default;
  server_builder(server_builder&& other) noexcept = default;
  server_builder& operator=(server_builder&& other) noexcept = default;

  inline server build() { return server(options_factory.options()); }

  program::options_factory options_factory;
};

}  // namespace proxy
