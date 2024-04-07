/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "aether/input/command_service.hpp"
#include "aether/input/commands/logs/logs.hpp"
#include "aether/interceptors/attach.hpp"
#include "aether/interceptors/examples/pokengine/pokengine.hpp"
#include "aether/interceptors/examples/teapot/teapot.hpp"
#include "aether/proxy/error/error.hpp"
#include "aether/proxy/server_builder.hpp"
#include "aether/util/any_invocable.hpp"
#include "aether/util/console.hpp"
#include "aether/util/generic_error.hpp"
#include "aether/util/result.hpp"
#include "aether/util/result_macros.hpp"

proxy::server make_server(int argc, char* argv[]) {
  proxy::server_builder builder;
  builder.options_factory.parse_cmdline(argc, argv);
  return builder.build();
}

// Program entry-point.
//
// Start the proxy server and wait for the user to stop it.
int main(int argc, char* argv[]) {
  try {
    proxy::server server = make_server(argc, argv);
    const program::options& options = server.options();

    interceptors::attach_default(server);

    interceptors::examples::pokengine_interceptor pokengine_interceptor;
    server.interceptors().attach_hub(pokengine_interceptor);

    if (proxy::result<void> res = server.start(); res.is_err()) {
      out::error::log("Failed to start server:", res);
      return 1;
    }

    if (!options.run_silent) {
      out::console::stream("Started running at ", server.endpoint_string(), out::manip::endl);

      // Only run the logs command.
      if (options.run_interactive) {
        input::command_service command_handler(std::cin, server);
        command_handler.run();
      } else if (options.run_logs) {
        out::console::log("Logs started. Press Ctrl+C (^C) to stop the server.");
        input::commands::logs logs_command;
        logs_command.attach_interceptors(server);
      }
    }

    server.await_stop();
    out::console::log("Server exited successfully.");
  }
  // Unexpected error.
  catch (const std::exception& ex) {
    out::error::stream("Unexpected exception: ", ex.what());
    return 1;
  }

  return 0;
}
