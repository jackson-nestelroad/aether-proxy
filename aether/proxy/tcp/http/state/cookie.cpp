/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "cookie.hpp"

namespace proxy::tcp::http {
    std::optional<cookie> cookie::parse_set_header(std::string_view header) {
        std::size_t separator = header.find('=');
        if (separator == std::string::npos) {
            return std::nullopt;
        }
        cookie result;
        std::size_t attribute_separator = header.find(';');
        result.name = util::string::trim(util::string::substring(header, 0, separator));
        result.value = util::string::trim(util::string::substring(header, separator + 1, attribute_separator));

        if (attribute_separator != std::string::npos) {
            for (std::size_t i = attribute_separator; i < header.size() && i != std::string::npos;) {
                ++i;
                std::size_t attribute_end = header.find(';', i);
                std::size_t separator = header.find('=', i);

                // No value
                if (separator >= attribute_end) {
                    result.attributes.emplace(util::string::trim(util::string::substring(header, i, attribute_end)), "");
                }
                else {
                    result.attributes.emplace(util::string::trim(util::string::substring(header, i, separator)), util::string::trim(util::string::substring(header, separator + 1, attribute_end)));
                }

                i = attribute_end;
            }
        }

        return result;
    }

    std::optional<std::string> cookie::get_attribute(std::string_view attribute) const {
        auto it = attributes.find(util::string::as_string(attribute));
        if (it == attributes.end()) {
            return std::nullopt;
        }

        return it->second;
    }
    void cookie::set_attribute(std::string_view attribute, std::string_view value) {
        auto it = attributes.find(util::string::as_string(attribute));
        if (it == attributes.end()) {
            attributes.emplace(attribute, value);
        }
        it->second = value;
    }

    std::optional<std::string> cookie::domain() const {
        auto it = attributes.find("Domain");
        if (it == attributes.end()) {
            return std::nullopt;
        }

        std::size_t start = 0;
        if (it->second[0] == '.') {
            ++start;
        }
        return it->second.substr(start);
    }

    std::string cookie::to_string() const {
        std::stringstream str;
        str << *this;
        return str.str();
    }

    std::ostream &operator<<(std::ostream &output, const cookie &cookie) {
        output << cookie.name << '=' << cookie.value;
        for (const auto &attribute_pair : cookie.attributes) {
            output << "; " << attribute_pair.first;
            if (!attribute_pair.second.empty()) {
                output << '=' << attribute_pair.second;
            }
        }
        return output;
    }
}