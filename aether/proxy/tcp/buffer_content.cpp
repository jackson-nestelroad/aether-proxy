/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "buffer_content.hpp"

namespace proxy::tcp {
    buffer_content::buffer_content() { }

    buffer_content::buffer_content(const buffer_content &other) {
        *this = other;
    }

    buffer_content &buffer_content::operator=(const buffer_content &other) {
        if (&other != this) {
            clear_content();
            copy_buffer(other.content);
        }
        return *this;
    }

    buffer_content::buffer_content(buffer_content &&other) noexcept {
        *this = std::move(other);
    }

    buffer_content &buffer_content::operator=(buffer_content &&other) noexcept {
        if (&other != this) {
            clear_content();
            copy_buffer(other.content);
        }
        return *this;
    }

    void buffer_content::copy_buffer(const streambuf &other) {
        std::size_t bytes_copied = boost::asio::buffer_copy(content.prepare(other.size()), other.data());
        content.commit(bytes_copied);
    }

    streambuf &buffer_content::get_content_buffer() {
        return content;
    }

    const streambuf &buffer_content::get_content_buffer() const {
        return content;
    }

    std::string buffer_content::copy_content_to_string() const {
        const auto &data = content.data();
        return std::string(boost::asio::buffers_begin(data), boost::asio::buffers_end(data));
    }

    std::string buffer_content::move_content_to_string() {
        return std::string(std::istreambuf_iterator<char>(&content), std::istreambuf_iterator<char>());
    }

    void buffer_content::clear_content() {
        content.consume(content.size());
    }

    std::ostream buffer_content::output_stream() {
        return std::ostream(&content);
    }

    std::istream buffer_content::input_stream() {
        return std::istream(&content);
    }

    std::size_t buffer_content::content_length() const {
        return content.size();
    }
}