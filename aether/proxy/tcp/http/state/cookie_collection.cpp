/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "cookie_collection.hpp"

namespace proxy::tcp::http {
    void cookie_collection::update(const cookie_collection &new_cookies) {
        for (const auto &[name, new_cookie] : new_cookies) {
            set(new_cookie);
        }
    }

    std::optional<std::reference_wrapper<cookie>> cookie_collection::get(std::string_view name) {
        auto it = cookies.find(util::string::as_string(name));
        if (it == cookies.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::optional<std::reference_wrapper<const cookie>> cookie_collection::get(std::string_view name) const {
        auto it = cookies.find(util::string::as_string(name));
        if (it == cookies.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    void cookie_collection::set(const cookie &cook) {
        auto it = cookies.find(cook.name);
        if (it == cookies.end()) {
            cookies.emplace(cook.name, cook);
        }
        else {
            it->second = cook;
        }
    }

    void cookie_collection::remove(const cookie &cook) {
        cookies.erase(cook.name);
    }

    std::size_t cookie_collection::size() const {
        return cookies.size();
    }

    bool cookie_collection::empty() const {
        return cookies.empty();
    }

    std::string cookie_collection::request_string() const {
        if (cookies.empty()) {
            return "";
        }

        std::ostringstream str;
        auto it = cookies.begin();
        str << it->second.request_string();
        
        for (++it; it != cookies.end(); ++it) {
            str << "; " << it->second.request_string();
        }
        return str.str();
    }

    cookie_collection cookie_collection::parse_request_header(std::string_view header) {
        cookie_collection result;
        for (std::size_t i = 0; i < header.length();) {
            ++i;
            std::size_t separator = header.find('=', i);
            std::size_t cookie_end = header.find(';', i);
            if (separator < cookie_end) {
                result.set({ util::string::trim(util::string::substring(header, i, separator)), util::string::trim(util::string::substring(header, separator + 1, cookie_end)) });
            }
            i = cookie_end;
        }
        return result;
    }
}