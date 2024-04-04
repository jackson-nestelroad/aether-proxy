/*********************************************

  Copyright (c) Jackson Nestelroad 2024
  jackson.nestelroad.com

*********************************************/

#include "stats.hpp"

#include <chrono>
#include <memory>

#include "aether/util/console.hpp"
#include "aether/util/signal_handler.hpp"

namespace input::commands {

void stats::run(const arguments_t& args, proxy::server& server, command_service& caller) {
  std::unique_ptr<util::signal_handler> signals;
  bool watch = false;
  if (args.size() > 0 && args[0] == "watch") {
    watch = true;
    signals = std::make_unique<util::signal_handler>(server.get_io_context());
    signals->wait([&]() { watch = false; });
  }

  if (watch) {
    out::user::log("Press Ctrl+C (^C) to stop watching stats.");
  }

  print_stats(server);
  while (watch) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    out::user::stream(out::manip::endl);
    print_stats(server);
  }
}

void stats::print_stats(proxy::server& server) {
  out::user::stream("Connections:\t\t", server.num_connections(), out::manip::endl);
  out::user::stream("SSL Certificates:\t", server.num_ssl_certificates(), out::manip::endl);
}

}  // namespace input::commands
