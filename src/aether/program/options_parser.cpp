/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "options_parser.hpp"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <cctype>
#include <functional>
#include <iterator>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "aether/proxy/error/exceptions.hpp"
#include "aether/util/console.hpp"
#include "aether/util/validate.hpp"

namespace program {

std::string options_parser::stored_option::generate_help_string(const options_parser::stored_option& option) {
  if (option.letter.has_value()) {
    std::string out = "-" + std::string(1, option.letter.value());
    if (option.name.has_value()) {
      out += out::string::stream(", --", option.name.value());
    }
    return out;
  } else {
    return out::string::stream("--", option.name.value());
  }
}

bool options_parser::string_to_bool(std::string_view str, bool def) {
  std::string lower;
  std::transform(str.begin(), str.end(), std::back_inserter(lower), [](unsigned char c) { return std::tolower(c); });
  if (lower == "true") {
    return true;
  } else if (lower == "false") {
    return false;
  } else {
    return def;
  }
}

std::string_view options_parser::bool_to_string(bool b) { return b ? "true" : "false"; }

int options_parser::parse(int argc, char* argv[]) {
  // Record required options.
  std::vector<bool> required(num_required_);

  int i;
  for (i = 1; i < argc; ++i) {
    std::string_view curr = {argv[i]};
    // End of options.
    if (curr == "--") {
      ++i;
      break;
    }
    // Not an option.
    if (curr[0] != '-') {
      throw option_exception(out::string::stream("Unknown option ", curr));
    }

    auto eq = curr.find('=');
    bool is_full_name = curr[1] == '-';

    // Find the option object matched by the option string.
    option_map_t::iterator matched;
    if (is_full_name) {
      std::string_view name_string = curr.substr(2, eq - 2);
      matched = std::find_if(option_map_.begin(), option_map_.end(),
                             [&name_string](const auto& it) { return it.second.name == name_string; });
    } else {
      char letter = curr[1];
      matched = std::find_if(option_map_.begin(), option_map_.end(),
                             [letter](const auto& it) { return it.second.letter == letter; });
    }

    if (matched == option_map_.end()) {
      throw option_exception(out::string::stream("Unknown option ", curr.substr(0, eq)));
    }

    auto& option = matched->second;

    if (option.required()) {
      required[option.required_id] = true;
    }

    if (is_full_name && eq != std::string::npos) {
      // Use value after the equal sign.
      option.parser(option, curr.substr(eq + 1));
    } else {
      // Don't use next value for boolean flag.
      if (option.is_boolean) {
        option.parser(option, "");
      } else {
        // Use next value for flag requiring a value.
        if (i == argc - 1) {
          throw option_exception("Missing value for option " + option.help_string);
        }
        option.parser(option, argv[++i]);
      }
    }
  }

  // Check if all required arguments were seen
  if (std::any_of(required.begin(), required.end(), [](bool b) { return !b; })) {
    throw option_exception("Missing 1 or more required arguments");
  }

  return i;
}

void options_parser::print_options() const {
  // Longest option will determine spaces between options and descriptions
  const auto& max = std::max_element(option_map_.begin(), option_map_.end(),
                                     [](const auto& a, const auto& b) { return a.first.size() < b.first.size(); });

  auto spaces = max->first.size() + 4;

  for (const auto& [help_string, option] : option_map_) {
    out::raw_stdout::stream(help_string);
    if (option.description.has_value()) {
      out::raw_stdout::stream(std::string(spaces - option.help_string.size(), ' '));
      if (option.required()) {
        out::raw_stdout::stream("[REQUIRED] ");
      }
      out::raw_stdout::stream(option.description.value());
    }

    if (option.default_value.has_value()) {
      out::raw_stdout::stream(std::endl, std::string(spaces, ' '), "Default = ", option.default_value.value());
    }
    out::raw_stdout::stream(std::endl);
  }
}
}  // namespace program
