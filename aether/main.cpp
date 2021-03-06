/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include <aether/proxy/server_builder.hpp>
#include <aether/util/console.hpp>
#include <aether/interceptors/attach.hpp>
#include <aether/input/command_service.hpp>
#include <aether/input/commands/logs/logs.hpp>

#include <aether/interceptors/examples/facebook/facebook.hpp>

/*
    Program entry-point.
    Start the proxy server and wait for the user to stop it.
*/
int main(int argc, char *argv[]) {
    try {
        proxy::server_builder builder;
        program::options &options = builder.options;
        options.parse_cmdline(argc, argv);

        proxy::server server = builder.build();

        interceptors::attach_default(server);

        interceptors::examples::facebook_interceptor facebook_interceptor;
        server.interceptors.attach_hub(facebook_interceptor);

        server.start();

        if (!options.run_silent) {
            out::console::stream("Started running at ", server.endpoint_string(), out::manip::endl);

            // Only run the logs command
            if (options.run_interactive) {
                input::command_service command_handler(std::cin, server);
                command_handler.run();
            }
            else if (options.run_logs) {
                out::console::log("Logs started. Press Ctrl+C (^C) to stop the server.");
                input::commands::logs logs_command;
                logs_command.attach_interceptors(server);
            }
        }

        server.await_stop();
        out::console::log("Server exited successfully.");
    }
    // Proxy error
    catch (const proxy::error::base_exception &ex) {
        out::error::log(ex.what());
        return 1;
    }
    // Unexpected error
    catch (const std::exception &ex) {
        out::error::stream("Unexpected error: ", ex.what());
        return 1;
    }

    return 0;
}