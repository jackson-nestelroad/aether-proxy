/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
#include <set>
#include <type_traits>
#include <optional>
#include <any>
#include <boost/lexical_cast.hpp>

#include <aether/util/console.hpp>
#include <aether/util/validate.hpp>

namespace program {
    /*
        Exception for parsing and handling command-line options.
    */
    class option_exception
        : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    /*
        Class for parsing command-line options attached to the program.
        Has functionality for default arguments, validation functions, and help message generation.
    */
    class options_parser {
    private:
        template <typename T>
        using validate_func = util::validate::validate_func<T>;
        
        template <typename In, typename Out>
        using convert_func = std::function<Out(In)>;
        
        // Function for parsing a string into another data type and storing it in its destination
        using parse_func = std::function<void(std::string_view)>;

    public:
        /*
            A single command-line option.
        */
        class option {
        private:
            // How the command-line option can be added
            // An option is "--option", a flag is "-f"
            
            // Both are listed as optional, but in reality, one or the other is required
            // This is enforced by the public methods for adding options
            std::optional<std::string> opt;
            std::optional<char> flag;

            std::optional<std::string> default_value;
            std::optional<std::string> description;
            parse_func parser;

            // Represents how the option is displayed in the help message
            // Option: --option
            // Flag: -f
            // Option + Flag: -f, --option
            std::string full_string;

            // Boolean values are handled differently, so we save whether or not this is a boolean option
            bool is_boolean;

            // If the option is required, this int will be >= 0
            // A unique indentifier for the option that is used to check that all required options
            // are found when parsing
            int required = -1;

            /*
                Generates the full_string attribute.
            */
            static std::string to_string(const std::optional<std::string_view> &opt, const std::optional<char> &flag);

            bool is_required() const;

            option(
                const std::optional<std::string_view> &opt,
                const std::optional<char> &flag,
                const std::optional<std::string_view> &default_value,
                const std::optional<std::string_view> &description,
                bool is_boolean
            )
                : opt(opt),
                flag(flag),
                default_value(default_value),
                description(description),
                parser(),
                is_boolean(is_boolean)
            { 
                full_string = to_string(opt, flag);
            };

        public:
            bool operator<(const option &other) const;

            friend class options_parser;
        };

    private:
        using option_map_t = std::set<option>;

        option_map_t option_map;
        
        // Number of required options
        // All num_required options must be found when parsing
        int num_required = 0;

        static bool string_to_bool(std::string_view str, bool def);
        static std::string_view bool_to_string(bool b);

        template <typename T>
        T default_converter(T val) {
            return val;
        }

        template <typename In, typename Out = In>
        void _add_option(
            const std::optional<std::string_view> &opt,
            const std::optional<char> &flag,
            Out *const &destination,
            const std::optional<In> &default_value,
            const std::optional<std::string_view> &description,
            const std::optional<validate_func<In>> &validate,
            const std::optional<convert_func<In, Out>> &converter,
            bool required
        ) {
            // Assign default value
            if (default_value.has_value()) {
                if constexpr (!std::is_convertible_v<In, Out>) {
                    *destination = converter.value()(default_value.value());
                }
                else {
                    *destination = converter.has_value() ? converter.value()(default_value.value()) : static_cast<Out>(default_value.value());
                }
            }
            try {
                option new_option(
                    opt,
                    flag,
                    default_value.has_value() ? boost::lexical_cast<std::string>(default_value.value()) : std::optional<std::string> { },
                    description,
                    false
                );

                if (required) {
                    new_option.required = num_required++;
                }

                std::string name = new_option.full_string;

                // Generate parsing function
                new_option.parser = [destination, validate, converter, name](std::string_view str) {
                    try {
                        In val = boost::lexical_cast<In>(str);
                        if (validate.has_value() && !(validate.value())(val)) {
                            throw boost::bad_lexical_cast();
                        }
                        
                        if constexpr (!std::is_convertible_v<In, Out>) {
                            *destination = converter.value()(val);
                        }
                        else {
                            *destination = converter.has_value() ? converter.value()(val) : static_cast<Out>(val);
                        }
                    }
                    catch (const boost::bad_lexical_cast &) {
                        throw option_exception("Invalid value for option " + name);
                    }
                };

                option_map.insert(new_option);
            }
            catch (const boost::bad_lexical_cast &) {
                throw option_exception("options_parser: Cannot convert type T to string");
            }
        }

        /*
            Adds a boolean option.
            Boolean options work slightly different because values are not required for them.
        */
        template <typename Out>
        void _add_option(
            const std::optional<std::string_view> &opt,
            const std::optional<char> &flag,
            Out *const &destination,
            const std::optional<bool> &default_value /* = false*/,
            const std::optional<std::string_view> &description,
            const std::optional<validate_func<bool>> & /*validate*/,
            const std::optional<convert_func<bool, Out>> &converter,
            bool required
        ) {
            bool def = default_value.has_value() ? default_value.value() : false;
            if constexpr (!std::is_convertible_v<bool, Out>) {
                *destination = converter.value()(def);
            }
            else {
                *destination = converter.has_value() ? converter.value()(def) : static_cast<Out>(def);
            }

            option new_option(
                opt,
                flag,
                bool_to_string(def),
                description,
                true
            );

            if (required) {
                new_option.required = num_required++;
            }

            // Parsing function is much simpler because no exceptions are thrown
            new_option.parser = [destination, converter, def](std::string_view str) {
                if constexpr (!std::is_convertible_v<bool, Out>) {
                    *destination = converter.value()(string_to_bool(str, true));
                }
                else {
                    *destination = converter.has_value() ? converter.value()(string_to_bool(str, true)) : static_cast<Out>(string_to_bool(str, true));
                }
            };

            option_map.insert(new_option);
        }

    public:
        /*
            Adds a command-line option that can be called with --opt or -f.
        */
        template <typename In, typename Out = In>
        void add_option(
            std::string_view opt,
            char flag,
            Out *const &destination,
            const std::optional<In> &default_value,
            const std::optional<std::string_view> &description,
            const std::optional<validate_func<In>> &validate,
            const std::optional<convert_func<In, Out>> &converter
        ) {
            _add_option(opt, flag, destination, default_value, description, validate, converter, false);
        }

        /*
            Adds a command-line option that can only be called with --opt.
        */
        template <typename In, typename Out = In>
        void add_option(
            std::string_view opt,
            Out *const &destination,
            const std::optional<In> &default_value,
            const std::optional<std::string_view> &description,
            const std::optional<validate_func<In>> &validate,
            const std::optional<convert_func<In, Out>> &converter
        ) {
            _add_option(opt, { }, destination, default_value, description, validate, converter, false);
        }

        /*
            Adds a command-line option that can only be called with -f.
        */
        template <typename In, typename Out = In>
        void add_option(
            char flag,
            Out *const &destination,
            const std::optional<In> &default_value,
            const std::optional<std::string_view> &description,
            const std::optional<validate_func<In>> &validate,
            const std::optional<convert_func<In, Out>> &converter
        ) {
            _add_option({ }, flag, destination, default_value, description, validate, converter, false);
        }

        /*
            Adds a required command-line option that can be called with --opt or -f.
        */
        template <typename In, typename Out = In>
        void add_option_required(
            std::string_view opt,
            char flag,
            Out *const &destination,
            const std::optional<In> &default_value,
            const std::optional<std::string_view> &description,
            const std::optional<validate_func<In>> &validate,
            const std::optional<convert_func<In, Out>> &converter
        ) {
            _add_option(opt, flag, destination, default_value, description, validate, converter, true);
        }

        /*
            Adds a required command-line option that can only be called with --opt.
        */
        template <typename In, typename Out = In>
        void add_option_required(
            std::string_view opt,
            Out *const &destination,
            const std::optional<In> &default_value,
            const std::optional<std::string_view> &description,
            const std::optional<validate_func<In>> &validate,
            const std::optional<convert_func<In, Out>> &converter
        ) {
            _add_option(opt, { }, destination, default_value, description, validate, converter, true);
        }

        /*
            Adds a required command-line option that can only be called with -f.
        */
        template <typename In, typename Out = In>
        void add_option_required(
            char flag,
            Out *const &destination,
            const std::optional<In> &default_value,
            const std::optional<std::string_view> &description,
            const std::optional<validate_func<In>> &validate,
            const std::optional<convert_func<In, Out>> &converter
        ) {
            _add_option({ }, flag, destination, default_value, description, validate, converter, true);
        }

        /*
            Parses the command-line options.
            Returns the index after the last one read.
            If all arguments were read, will return argc.
        */
        int parse(int argc, char *argv[]) const;

        /*
            Prints the options and their descriptions to out::console.
        */
        void print_options() const;
    };
}