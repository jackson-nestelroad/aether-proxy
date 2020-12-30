/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <string>

#include <aether/proxy/types.hpp>

namespace proxy::tcp {
    /*
        Base class for a message that uses a proxy::streambuf buffer
            to represent the internal message content.
    */
    class buffer_content {
    private:
        void copy_buffer(const streambuf &other);

    protected:
        streambuf content;

    public:
        buffer_content();
        buffer_content(const buffer_content &other);
        buffer_content &operator=(const buffer_content &other);
        buffer_content(buffer_content &&other) noexcept;
        buffer_content &operator=(buffer_content &&other) noexcept;

        /*
            Returns a reference to the content buffer.
        */
        streambuf &get_content_buffer();

        /*
            Returns a constant reference to the content buffer.
        */
        const streambuf &get_content_buffer() const;

        /*
            Copies the content of the message into a string instance.
        */
        std::string copy_content_to_string() const;

        /*
            Reads the content of the message into a string instance,
                removing the contents from the internal buffer.
        */
        std::string move_content_to_string();

        /*
            Copies the content data to an iterator location.
        */
        template <typename Iterator>
        void copy_content(Iterator dest) const {
            auto data = content.data();
            std::copy(boost::asio::buffers_begin(data), boost::asio::buffers_end(data), dest);
        }

        /*
            Moves the content data to an iterator location, permanently.
        */
        template <typename Iterator>
        void move_content(Iterator dest) {
            std::copy(std::istreambuf_iterator<char>(&content), std::istreambuf_iterator<char>(), dest);
        }

        /*
            Clears all of the content in the content buffer.
        */
        void clear_content();

        /*
            Returns an output stream that adds to the content buffer.
        */
        std::ostream output_stream();

        /*
            Returns an input stream that reads from the content buffer.
        */
        std::istream input_stream();

        /*
            Returns the number of bytes in the content buffer.
        */
        std::size_t content_length() const;
    };
}