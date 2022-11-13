/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "logs.hpp"

#include <memory>

#include "aether/interceptors/attach.hpp"
#include "aether/interceptors/http/http_logger.hpp"
#include "aether/util/console.hpp"

namespace input::commands {

void logs::run(const arguments_t& args, proxy::server& server, command_service& caller) {
  out::user::log("Logs started. Press Ctrl+C (^C) to stop logging.");
  attach_interceptors(server);
  signals_ = std::make_unique<util::signal_handler>(server.get_io_context());
  signals_->wait([this]() { blocker_.unblock(); });
  blocker_.block();
  detach_interceptors(server);
  signals_.reset();
}

void logs::attach_interceptors(proxy::server& server) {
  http_id_ = server.interceptors().http.attach<interceptors::http_logger<out::safe_console>>();
  server.enable_logs();
}

void logs::detach_interceptors(proxy::server& server) {
  server.interceptors().http.detach(http_id_);
  server.disable_logs();
}

}  // namespace input::commands
