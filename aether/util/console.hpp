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
#include <mutex>

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
        Static std::mutex for a given std::ostream instance.
        Mutexes are NOT copyable or movable, so logging services must inherit the static
            mutex from this class.
        This is also how multiple logging services can print to the same output stream
            across multiple threads and services.
    */
    template <std::ostream *strm>
    class logging_mutex {
    protected:
        static std::mutex log_mutex;

    public:
        static void lock() {
            logging_mutex<strm>::log_mutex.lock();
        }

        static void unlock() {
            logging_mutex<strm>::log_mutex.unlock();
        }
    };

    template <std::ostream *strm>
    std::mutex logging_mutex<strm>::log_mutex;

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

    /*
        Uses stream-specific mutex to safely output data across multiple threads.
    */
    template <std::ostream *strm>
    class safe_base_stream {
    private:
        safe_base_stream() = delete;
        ~safe_base_stream() = delete;

        inline static void _log(void) {
            logging_mutex<strm>::unlock();
        }

        template <typename T>
        static void _log(const T &t) {
            *strm << t << std::endl;
            logging_mutex<strm>::unlock();
        }

        template <typename T, typename... Ts>
        static void _log(const T &t, const Ts &... ts) {
            *strm << t << ' ';
            _log(ts...);
        }

        inline static void _stream() {
            logging_mutex<strm>::unlock();
        }

        template <typename T, typename... Ts>
        static void _stream(const T &t, const Ts &... ts) {
            *strm << t;
            _stream(ts...);
        }

        template <typename... Ts>
        static void _stream(const manip_t &manip, const Ts &... ts) {
            (*manip)(*strm);
            _stream(ts...);
        }

    public:
        inline static void log(void) {
            logging_mutex<strm>::lock();
            *strm << std::endl;
            logging_mutex<strm>::unlock();
        }

        template <typename T>
        static void log(const T &t) {
            logging_mutex<strm>::lock();
            *strm << t << std::endl;
            logging_mutex<strm>::unlock();
        }

        template <typename T, typename... Ts>
        static void log(const T &t, const Ts &... ts) {
            logging_mutex<strm>::lock();
            _log(t, ts...);
        }

        inline static void stream(void) { }

        template <typename T, typename... Ts>
        static void stream(const T &t, const Ts &... ts) {
            logging_mutex<strm>::lock();
            *strm << t;
            _stream(ts...);
        }

        template <typename... Ts>
        static void stream(const manip_t &manip, const Ts &... ts) {
            logging_mutex<strm>::lock();
            (*manip)(*strm);
            _stream(ts...);
        }
    };

    // Explicit instantiation declarations for standard output streams

    using console = base_stream<&std::cout>;
    using error = base_stream<&std::cerr>;

    using safe_console = safe_base_stream<&std::cout>;
    using safe_error = safe_base_stream<&std::cerr>;

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