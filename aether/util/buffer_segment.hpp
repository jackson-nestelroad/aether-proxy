/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <iterator>

namespace util::buffer {
    /*
        Utility class for managing data that may need to be read from an
            input stream multiple times to meet the complete condition.
    */
    class buffer_segment {
    private:
        bool is_complete;
        std::string read_data;
        std::size_t num_bytes_read;

    public:
        buffer_segment();

        std::string &data_ref();
        std::string export_data() const;
        bool complete() const;
        std::size_t bytes_read() const;
        void reset();
        void mark_as_complete();
        void mark_as_incomplete();

        template <typename Iterator>
        void copy_data(const Iterator &dest) const {
            std::copy(read_data.begin(), read_data.end(), dest);
        }

        bool read_bytes(std::istream &in, std::size_t bytes);
        bool read_up_to_bytes(std::istream &in, std::size_t bytes);
        bool read_until(std::istream &in, char delim);
        bool read_until(std::istream &in, std::string_view delim);
    };
}