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
        Extracts a line from the buffer using std::getline.
    */
    std::string extract_line(streambuf &buf);

    /*
        Extracts a given number of bytes from the buffer.
    */
    std::string extract_bytes(streambuf &buf, std::size_t bytes);

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