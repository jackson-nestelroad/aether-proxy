/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/types.hpp>
#include <aether/proxy/error/exceptions.hpp>
#include <aether/util/buffer_segment.hpp>

namespace proxy::tcp::tls::handshake {
    /*
        Reads the raw handshake data for it to be parsed later.
    */
    class handshake_reader {
    private: 
        util::buffer::buffer_segment header_segment;
        util::buffer::buffer_segment record_segment;
        std::uint16_t length;
        bool known_length;

    public:
        static constexpr std::size_t record_header_length = 5;
        handshake_reader();
        
        /*
            Reads the raw ClientHello message from the input stream.
            Returns the number of bytes needed to complete the record.
            Returns 0 if finished.
        */
        std::size_t read(std::istream &in);
        
        byte_array get_bytes() const;
        void reset();
    };
}