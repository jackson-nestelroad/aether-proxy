/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <string>
#include <fstream>
#include <optional>
#include <unordered_map>
#include <boost/filesystem.hpp>

#include <aether/util/singleton.hpp>
#include <aether/util/string.hpp>

namespace program {
    class properties_exception
        : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    /*
        Parser and reader for a file of key-value pairs.
    */
    class properties {
    private:
        std::unordered_map<std::string, std::string> props;

    public:
        /*
            Get the value associated to a property key.
        */
        std::optional<std::string_view> get(const std::string &key) const;

        /*
            Parse the values of a flat .properties file.
        */
        void parse_file(const std::string &file_path);
    };
}