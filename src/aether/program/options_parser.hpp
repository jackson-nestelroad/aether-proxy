/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/lexical_cast.hpp>
#include <functional>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "aether/proxy/error/exceptions.hpp"
#include "aether/util/any_invocable.hpp"
#include "aether/util/validate.hpp"

namespace program {
// Exception for parsing and handling command-line options.
class option_exception : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

// A command-line option to be added to the parser.
template <typename In, typename Out = In>
struct command_line_option {
  std::optional<std::string> name;
  std::optional<char> letter;
  Out* const& destination;
  bool required;
  std::optional<In> default_value;
  std::optional<std::string> description;
  std::optional<util::validate::validate_func_t<In>> validate;
  std::optional<util::any_invocable<Out(In&&)>> converter;
};

// Class for parsing command-line options attached to the program.
// Has functionality for default arguments, validation functions, and help message generation.
class options_parser {
 private:
  // A single command-line option.
  // This struct effectively erases the type of the option added by the user so that all options may be stored in the
  // same map.
  struct stored_option {
   public:
    std::optional<std::string> name;
    std::optional<char> letter;
    std::optional<std::string> default_value;
    std::optional<std::string> description;

    // Boolean values are handled differently, so we save whether or not this is a boolean option.
    bool is_boolean;

    // If the option is required, this int will be >= 0.
    // A unique indentifier for the option that is used to check that all required options are found when parsing.
    int required_id = -1;

    // The option displayed in the help message.
    std::string help_string;

    util::any_invocable<void(const stored_option&, std::string_view)> parser;

    inline stored_option(std::optional<std::string> name, std::optional<char> letter,
                         std::optional<std::string> default_value, std::optional<std::string> description,
                         bool is_boolean)
        : name(std::move(name)),
          letter(std::move(letter)),
          default_value(std::move(default_value)),
          description(std::move(description)),
          is_boolean(is_boolean) {}
    stored_option() = default;
    ~stored_option() = default;
    stored_option(const stored_option& other) = delete;
    stored_option& operator=(const stored_option& other) = delete;
    stored_option(stored_option&& other) noexcept = default;
    stored_option& operator=(stored_option&& other) noexcept = default;

    inline bool required() const { return required_id >= 0; }

    // Generates how the option is displayed in the help message.
    static std::string generate_help_string(const stored_option& option);
  };

 public:
  template <typename In, typename Out = In>
  void add_option(command_line_option<In, Out>&& option) {
    validate_option(option);
    add_option_internal(std::move(option));
  }

  // Parses the command-line options.
  // Returns the index after the last one read.
  // If all arguments were read, will return argc.
  int parse(int argc, char* argv[]);

  // Prints the options and their descriptions to out::console.
  void print_options() const;

 private:
  static bool string_to_bool(std::string_view str, bool def);
  static std::string_view bool_to_string(bool b);

  template <typename In, typename Out>
  void validate_option(command_line_option<In, Out>& option) {
    if (!option.name.has_value() && !option.letter.has_value()) {
      throw option_exception{"One of option name or letter must be specified"};
    }

    if constexpr (!std::is_convertible_v<In, Out>) {
      if (!option.converter.has_value()) {
        throw option_exception{"No converter defined for option with non-convertible output type"};
      }
    }
  }

  template <typename In, typename Out = In>
  void add_option_internal(command_line_option<In, Out> option) {
    std::string default_value_str = option.default_value.has_value()
                                        ? boost::lexical_cast<std::string>(std::move(option.default_value).value())
                                        : std::string{};
    // Assign default value.
    if (option.default_value.has_value()) {
      if constexpr (!std::is_convertible_v<In, Out>) {
        *option.destination = option.converter.value()(std::move(option.default_value).value());
      } else {
        *option.destination = option.converter.has_value()
                                  ? option.converter.value()(std::move(option.default_value).value())
                                  : static_cast<Out&&>(option.default_value.value());
      }
    }

    stored_option new_option(std::move(option.name), std::move(option.letter), default_value_str,
                             std::move(option.description), false);
    new_option.help_string = stored_option::generate_help_string(new_option);

    if (option.required) {
      new_option.required_id = num_required_++;
    }

    // Generate parsing function.
    new_option.parser = [validate = std::move(option.validate), converter = std::move(option.converter),
                         destination = option.destination](const stored_option& option, std::string_view str) mutable {
      try {
        In val = boost::lexical_cast<In>(str);
        if (validate.has_value() && !(validate.value())(val)) {
          throw boost::bad_lexical_cast();
        }

        if constexpr (!std::is_convertible_v<In, Out>) {
          *destination = converter.value()(std::move(val));
        } else {
          *destination = converter.has_value() ? converter.value()(std::move(val)) : static_cast<Out&&>(val);
        }
      } catch (const boost::bad_lexical_cast&) {
        throw option_exception("Invalid value for option " + option.help_string);
      }
    };

    std::string key = new_option.help_string;
    option_map_.emplace(std::move(key), std::move(new_option));
  }

  // Adds a boolean option.
  // Boolean options work slightly different because values are not required for them.
  template <typename Out>
  void add_option_internal(command_line_option<bool, Out> option) {
    bool def = option.default_value.value_or(false);
    if constexpr (!std::is_convertible_v<bool, Out>) {
      *option.destination = option.converter.value()(std::move(def));
    } else {
      *option.destination =
          option.converter.has_value() ? option.converter.value()(std::move(def)) : static_cast<Out&&>(def);
    }

    stored_option new_option(std::move(option.name), std::move(option.letter), std::string(bool_to_string(def)),
                             std::move(option.description), true);
    new_option.help_string = stored_option::generate_help_string(new_option);

    if (option.required) {
      new_option.required_id = num_required_++;
    }

    // Parsing function is much simpler because no exceptions are thrown.
    new_option.parser = [validate = std::move(option.validate), converter = std::move(option.converter),
                         destination = option.destination](const stored_option& option, std::string_view str) mutable {
      bool boolean_value = string_to_bool(str, true);
      if (validate.has_value() && !(validate.value())(boolean_value)) {
        throw option_exception("Invalid value for option " + option.help_string);
      }

      if constexpr (!std::is_convertible_v<bool, Out>) {
        *destination = converter.value()(std::move(boolean_value));
      } else {
        *destination =
            converter.has_value() ? converter.value()(std::move(boolean_value)) : static_cast<Out&&>(boolean_value);
      }
    };

    option_map_.emplace(new_option.help_string, std::move(new_option));
  }

  using option_map_t = std::map<std::string, stored_option>;

  option_map_t option_map_;

  // Number of required options.
  // All "num_required_" options must be found when parsing.
  int num_required_ = 0;
};

}  // namespace program
