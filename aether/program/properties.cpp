/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "properties.hpp"

namespace program {
    std::optional<std::string> properties::get(const std::string &key) const {
        auto it = props.find(key.data());
        if (it == props.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    void properties::parse_file(const std::string &file_path) {
        std::fstream file(file_path, std::fstream::in);
        if (!file) {
            throw properties_exception { "Could not open properties file \"" + file_path + "\" for reading." };
        }

        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty() && line.at(0) != '#') {
                auto split_loc = line.find('=');
                if (split_loc == 0) {
                    throw properties_exception { "Malformed property \"" + line + "\"." };
                }
                if (split_loc == std::string::npos) {
                    throw properties_exception { "Property \"" + line + "\" does not have a value." };
                }
                props.emplace(line.substr(0, split_loc), line.substr(split_loc + 1));
            }
        }
    }
}