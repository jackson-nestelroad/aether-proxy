/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <aether/proxy/tcp/tls/handshake/client_hello.hpp>
#include <aether/proxy/error/exceptions.hpp>

namespace proxy::tcp::tls::handshake {
    class parser {
    private: 
        byte_array bytes;
        std::uint16_t length;
        std::uint16_t remaining;
        bool known_length;

        std::size_t peek(std::istream &in, std::size_t num_bytes);
        void unget(std::istream &in, std::size_t num_bytes);

    public:
        parser();
        
        /*
            Reads the raw ClientHello message from the input stream.
            Returns the number of bytes needed to complete the record.
            Returns 0 if finished.
        */
        std::size_t read(std::istream &in);
        
        byte_array get_bytes() const;
        std::size_t get_remaining() const;
        bool is_finished() const;
    };
}