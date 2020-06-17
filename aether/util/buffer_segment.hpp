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
        // Data cannot be read if the segment is marked as complete
        bool is_complete;

        std::string read_data;
        std::size_t num_bytes_read;

    public:
        buffer_segment();

        /*
            Returns a reference to the data owned by the segment.
        */
        std::string &data_ref();

        /*
            Returns a copy of the data read by the segment.
        */
        std::string export_data() const;

        /*
            Checks if the segment is marked as complete.
            The segment is marked as complete when an input reader
                method fulfills its completion condition.
        */
        bool complete() const;

        /*
            Returns the number of bytes read into the buffer segment.
        */
        std::size_t bytes_read() const;

        /*
            Resets all data and flags.
        */
        void reset();

        void mark_as_complete();
        void mark_as_incomplete();

        /*
            Copies the segment data to an iterator location.
        */
        template <typename Iterator>
        void copy_data(const Iterator &dest) const {
            std::copy(read_data.begin(), read_data.end(), dest);
        }

        /*
            Reads a given number of bytes from the stream.
            Marked as complete if the exact number of bytes are read
                in this read session. This method is not stateful,
                so it will not account for the results of previous reads.
        */
        bool read_bytes(std::istream &in, std::size_t bytes);

        /*
            Reads from the stream until the total number of bytes read
                matches the number passed to this method.
            This method is stateful, so it will account for the results
                of previous reads.
            This method will fail if bytes_read is greater than bytes.
        */
        bool read_up_to_bytes(std::istream &in, std::size_t bytes);

        /*
            Reads from the stream until a character delimiter is found.
        */
        bool read_until(std::istream &in, char delim);

        /*
            Reads from the stream until a multi-character delimiter is found.
        */
        bool read_until(std::istream &in, std::string_view delim);
    };
}