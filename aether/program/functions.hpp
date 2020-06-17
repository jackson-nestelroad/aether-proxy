/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <string>
#include <thread>

#include <aether/program/options.hpp>
#include <aether/program/options_parser.hpp>
#include <aether/proxy/types.hpp>
#include <aether/proxy/connection/base_connection.hpp>
#include <aether/proxy/tcp/http/http1/http_parser.hpp>
#include <aether/util/console.hpp>
#include <aether/util/validate.hpp>

namespace program {
    /*
        Parses all command-line options for this proxy server program.
        Returns a struct of all command-line options.
    */
    options parse_cmdline_options(int argc, char *argv[]);
}