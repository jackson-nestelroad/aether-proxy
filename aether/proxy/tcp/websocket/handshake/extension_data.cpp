/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "extension_data.hpp"

namespace proxy::tcp::websocket::handshake {
    extension_data::extension_data(const std::string &name)
        : name(name)
    { }

    std::string extension_data::get_name() const {
        return name;
    }

    void extension_data::set_name(const std::string &name) {
        this->name = name;
    }

    bool extension_data::has_param(const std::string &name) const {
        return params.find(name) != params.end();
    }

    std::string extension_data::get_param(const std::string &name) const {
        const auto &it = params.find(name);
        if (it == params.end()) {
            throw error::websocket::extension_param_not_found_exception { "Extension parameter \"" + name + "\" was not found" };
        }
        return it->second;
    }

    void extension_data::set_param(const std::string &name, const std::string &value) {
        params[name] = value;
    }

    extension_data extension_data::from_header_value(const std::string &header) {
        std::size_t multi_extensions = header.find_first_of(extension_delim);
        if (multi_extensions != std::string::npos) {
            throw error::websocket::invalid_extension_string_exception { "Multiple extensions present in single string" };
        }

        const auto &parsed = util::string::split_trim(header, param_delim);
        std::size_t size = parsed.size();
        if (size < 1) {
            throw error::websocket::invalid_extension_string_exception { "No extension name found" };
        }

        extension_data result(parsed[0]);
        for (std::size_t i = 1; i < size; ++i) {
            const std::string &curr = parsed[i];
            std::size_t assign_pos = curr.find_first_of(assign_delim);
            if (assign_pos == std::string::npos) {
                result.set_param(curr);
            }
            else {
                result.set_param(curr.substr(0, assign_pos), curr.substr(assign_pos + 1));
            }
        }

        return result;
    }

    std::ostream &operator<<(std::ostream &out, const extension_data &ext) {
        out << ext.name;
        for (const auto &[param, value] : ext.params) {
            out << extension_data::param_delim << ' ' << param << extension_data::assign_delim << value;
        }
        return out;
    }

    std::ostream &operator<<(std::ostream &out, const std::vector<extension_data> &ext_list) {
        bool first = true;
        for (const auto &ext : ext_list) {
            if (first) {
                out << ext;
                first = false;
            }
            else {
                out << extension_data::extension_delim << ' ' << ext;
            }
        }
        return out;
    }
}