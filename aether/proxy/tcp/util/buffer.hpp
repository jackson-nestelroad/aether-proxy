/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>
#include <string>
#include <string_view>

#include <aether/proxy/types.hpp>

// Helper functions for working with boost::asio::streambuf and std::istream

namespace proxy::tcp::buffer {
    /*
        Reads a given number of bytes from the input stream and stores the result in the destination string.
        Puts read characters back into the stream if number of bytes cannot be reached.
    */
    bool read_bytes(std::istream &in, std::string &dest, std::size_t bytes);

    /*
        Reads up to a givevn number of bytes from the input stream and stores the result in the destination string.
    */
    std::size_t read_up_to_bytes(std::istream &in, std::string &dest, std::size_t bytes);

    /*
        Reads the input stream up to a character delimiter and stores the result in the destination string.
        Puts read characters back into the stream if delimiter is not found.
    */
    bool read_until(std::istream &in, std::string &dest, char delim);

    /*
        Reads the input stream up to a string delimiter and stores the result in the destination string.

    */
    bool read_until(std::istream &in, std::string &dest, std::string_view delim);
}