/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "options_parser.hpp"

namespace program {
    std::string options_parser::option::to_string(const std::optional<std::string> &opt, const std::optional<char> &flag) {
        if (flag.has_value()) {
            std::string out = "-" + std::string(1, flag.value());
            if (opt.has_value()) {
                out += ", --" + opt.value();
            }
            return out;
        }
        else {
            return "--" + opt.value();
        }
    }

    bool options_parser::option::is_required() const {
        return required >= 0;
    }

    bool options_parser::option::operator<(const option &other) const {
        return full_string < other.full_string;
    }

    bool options_parser::string_to_bool(const std::string &str, bool def) {
        std::string lower;
        std::transform(str.begin(), str.end(), std::back_inserter(lower), [](unsigned char c) {
            return std::tolower(c);
        });
        if (lower == "true") {
            return true;
        }
        else if (lower == "false") {
            return false;
        }
        else {
            return def;
        }
    }

    std::string options_parser::bool_to_string(bool b) {
        return b ? "true" : "false";
    }
    
    int options_parser::parse(int argc, char *argv[]) const {
        // Record required options
        std::vector<bool> required(num_required);

        int i;
        for (i = 1; i < argc; ++i) {
            std::string curr = argv[i];
            // End of options
            if (curr == "--") {
                ++i;
                break;
            }
            // Not an option
            if (curr[0] != '-') {
                throw option_exception("Unknown option " + curr);
            }

            auto eq = curr.find('=');
            bool is_option = curr[1] == '-';
            
            // Find the option object matched by the option string
            option_map_t::iterator matched;
            if (is_option) {
                std::string option_string = curr.substr(2, eq - 2);
                matched = std::find_if(option_map.begin(), option_map.end(),
                    [&option_string](const auto &option) {
                        return option.opt == option_string;
                    });
            }
            else {
                char flag_char = curr[1];
                matched = std::find_if(option_map.begin(), option_map.end(),
                    [flag_char](const auto &option) {
                        return option.flag == flag_char;
                    });
            }

            if (matched == option_map.end()) {
                throw option_exception("Unknown option " + curr.substr(0, eq));
            }

            if (matched->is_required()) {
                required[matched->required] = true;
            }

            if (is_option) {
                // Use value after the equal sign
                matched->parser(curr.substr(eq + 1));
            }
            else {
                // Don't use next value for boolean flag
                if (matched->is_boolean) {
                    matched->parser("");
                }
                else {
                    // Use next value for flag requiring a value
                    if (i == argc - 1) {
                        throw option_exception("Missing value for option " + matched->full_string);
                    }
                    matched->parser(argv[++i]);
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
        const auto &max = std::max_element(option_map.begin(), option_map.end(),
            [](const auto &a, const auto &b) {
                return a.full_string < b.full_string;
            });

        auto spaces = max->full_string.size() + 4;
        
        for (const auto &option : option_map) {
            out::console::stream(option.full_string);
            if (option.description.has_value()) {
                out::console::stream(std::string(spaces - option.full_string.size(), ' '));
                if (option.is_required()) {
                    out::console::stream("[REQUIRED] ");
                }
                out::console::stream(option.description.value());
            }
            
            if (option.default_value.has_value()) {
                out::console::stream(std::endl, std::string(spaces, ' '), "Default = ", option.default_value.value());
            }
            out::console::stream(std::endl);
        }
    }
}