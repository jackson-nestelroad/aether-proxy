/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <iterator>

#include <aether/proxy/types.hpp>

namespace util::buffer {
    /*
        Base attributes and methods for managing data that may be collected
            from an input buffer or stream in one or more method calls.
    */
    class base_segment {
    protected:
        // Data cannot be read if the segment is marked as complete
        bool is_complete;

        std::string committed_data;

        std::string buffer_data;
        std::size_t num_bytes_read;

        base_segment();

        // Moves buffer_data to committed_data
        void commit_buffer();

    public:
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
            Returns the number of bytes committed to the buffer segment.
        */
        std::size_t size() const;

        /*
            Returns the number of bytes in the buffer (non-committed data).
        */
        std::size_t buffer_size() const;

        /*
            Resets all data and flags.
        */
        void reset();

        /*
            Commits all currently buffered data to be exported and marks the segment
                as complete.
        */
        void commit();

        void mark_as_incomplete();
        void mark_as_complete();

        /*
            Copies the segment data to an iterator location.
        */
        template <typename Iterator>
        void copy_data(const Iterator &dest) const {
            std::copy(committed_data.begin(), committed_data.end(), dest);
        }
    };

    /*
        Utility class for managing data that may need to be read from an
            input stream multiple times to meet the complete condition.
        Permanently removes data from the stream.
    */
    class buffer_segment : 
        public base_segment {
    public:
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

    /*
        Utility class for managing data that may need to be read from a
            constant buffer multiple times to meet the complete condition.
        Does not remove data when reading.
    */
    class const_buffer_segment 
        : public base_segment {
    public:
        /*
            Reads from the stream until the total number of bytes in the buffer
                matches the number passed to this method.
            This method is stateful, so it will account for the results
                of previous reads.
            This method will fail if bytes_read is greater than bytes.
        */
        bool read_up_to_bytes(proxy::const_streambuf &buf, std::size_t bytes, std::size_t size);
    };
}