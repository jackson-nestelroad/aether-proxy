/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <string>

#include "aether/program/options.hpp"
#include "aether/program/options_parser.hpp"

namespace program {

class options_factory {
 public:
  // Parses all command-line options according to the internal configuration of the instance.
  void parse_cmdline(int argc, char* argv[]);

  inline program::options& options() { return options_; }

 private:
  program::options options_;
  options_parser parser_;
  bool options_added_ = false;

  std::string command_name_;
  std::string usage_;

  // Add all options to the parser.
  void add_options();

  // Print help information for this command to the command-line.
  void print_help() const;
};

}  // namespace program
