/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <memory>

// Helper functions for data output to std::cout and std::cerr

namespace out {
    // Type for function template stream manipulators
    using manip_t = std::ostream & (*)(std::ostream &);

    /*
        Interface to function template stream manipulators to pass to out::base_stream::*.
        These manipulators are typically templates, so they must instantiated with the
            std::ostream type to be used in variadic function templates.
    */
    struct manip {
        static const manip_t endl;
        static const manip_t ends;
        static const manip_t flush;
    };

    /*
        Variadic template functions for stream output.
    */
    template <std::ostream *strm>
    class base_stream {
    private:
        base_stream() = delete;
        ~base_stream() = delete;
    public:
        inline static void log(void) {
            *strm << std::endl;
        }

        // ::log puts spaces between values and automatically enters a newline

        template <typename T>
        static void log(const T &t) {
            *strm << t << std::endl;
        }

        template <typename T, typename... Ts>
        static void log(const T &t, const Ts &... ts) {
            *strm << t << ' ';
            log(ts...);
        }

        // ::stream does not put any spaces between values and does not automatically insert newlines
        // Can also use stream manipulators via std:: or out::manip::

        inline static void stream(void) { }

        template <typename T, typename... Ts>
        static void stream(const T &t, const Ts &... ts) {
            *strm << t;
            stream(ts...);
        }

        template <typename... Ts>
        static void stream(const manip_t &manip, const Ts &... ts) {
            (*manip)(*strm);
            stream(ts...);
        }
    };

    // Explicit instantiation declaration for standard output streams
    using console = base_stream<&std::cout>;
    using error = base_stream<&std::cerr>;

    /*
        Thin wrapper for string concatenation using std::stringstream.
        Provides variadic template functions.
    */
    class string {
    private:
        string() = delete;
        ~string() = delete;

        inline static std::string _stream(std::stringstream &str) {
            return str.str();
        }

        template <typename T, typename... Ts>
        static std::string _stream(std::stringstream &str, const T &t, const Ts &... ts) {
            str << t;
            return _stream(str, ts...);
        }

    public:
        inline static std::string stream(void) {
            return "";
        }

        template <typename T, typename... Ts>
        static std::string stream(const T &t, const Ts &... ts) {
            std::stringstream str;
            return _stream(str, t, ts...);
        }
    };
}