/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>
#include <memory>
#include <mutex>
#include <boost/date_time.hpp>

#include <aether/util/singleton.hpp>

// Helper functions for data output

#ifdef AETHER_USE_STDERR
#define STDERR_FOR_LOG_INTERFACES std::cerr
#else
#define STDERR_FOR_LOG_INTERFACES std::cout
#endif

#define LOG_INTERFACES(X) \
X(console,      "[CONSOLE]", std::cout, false, true) \
X(error,        "  [ERROR]", STDERR_FOR_LOG_INTERFACES, false, true) \
X(warn,         "   [WARN]", std::cout, false, true) \
X(debug,        "  [DEBUG]", std::cout, true, true) \
X(trace,        "  [TRACE]", std::cout, true, true) \
X(raw_stdout,   " [STDOUT]", std::cout, false, false) \
X(raw_stderr,   " [STDERR]", STDERR_FOR_LOG_INTERFACES, false, false) \
X(user,         "   [USER]", terminal, false, false)

namespace out {
    /*
        Acts as another interface to std::cout so that if std::cout is silenced
            or redirected, the terminal is still accessible.
        This stream is not guaranteed to be a terminal if stdout is redirected externally.
    */
    extern std::ostream terminal;

    /*
        A buffer that allows no reading or writing.
    */
    class nullbuf : public std::streambuf {
    public:
        inline int overflow(int c) override {
            return std::char_traits<char>::not_eof(c);;
        }

        inline std::streamsize xsputn(const char *s, std::streamsize n) override {
            return 0;
        }
    };

    /*
        An output stream that allows no writing.
    */
    class onullstream : public std::ostream {
    private:
        nullbuf buf;

    public:
        inline onullstream()
            : std::ostream(&buf)
        { }
    };

    /*
        Object for managing all logging activities.
        Makes changes to global streams that are in effect for the lifetime of the object.
    */
    struct logging_manager {
    private:
        bool silenced;
        onullstream cnull;
        std::unique_ptr<std::ofstream> file_stream;
        std::streambuf *original_cout_buf;
        std::streambuf *original_cerr_buf;

        void reset_buffers();
        void reset_flags();

    public:
        logging_manager();
        ~logging_manager();

        /*
            Redirects the output streams to a file.
        */
        void redirect_to_file(const std::string &file_name);

        /*
            Unsyncs the output streams from stdio.
        */
        void unsync_with_stdio();

        /*
            Silences the output streams.
        */
        void silence();

        /*
            Unsilences the output stream, setting their buffers back to whatever they were before.
        */
        void unsilence();

        /*
            Resets the output streams.
        */
        void reset();
    };

    // Type for function template stream manipulators
    using manip_t = std::ostream &(*)(std::ostream &);

    /*
        Interface to function template stream manipulators to pass to stream interfaces.
        These manipulators are typically templates, so they must instantiated with the
            std::ostream type to be used in variadic template functions.
    */
    struct manip {
        static const manip_t endl;
        static const manip_t ends;
        static const manip_t flush;
    };

    /*
        Static std::mutex for a given std::ostream instance.
        Protects write operations on the same output stream object.
    */
    template <std::ostream *strm>
    class logging_mutex {
    protected:
        static std::mutex log_mutex;

    public:
        static std::lock_guard<std::mutex> acquire_lock_guard() {
            logging_mutex<strm>::log_mutex.lock();
            return { logging_mutex<strm>::log_mutex, std::adopt_lock };
        }
    };

    template <std::ostream *strm>
    std::mutex logging_mutex<strm>::log_mutex;

    /*
        Enumeration type for each type of log interface.
    */
    enum class log_type {
#define CREATE_LOG_ENUM(name, prefix, stream, debug_only, use_log_class) name,
        LOG_INTERFACES(CREATE_LOG_ENUM)
#undef CREATE_LOG_ENUM
    };

    /*
        Returns the prefix string for the given log type.
    */
    constexpr std::string_view get_log_prefix(log_type type) {
        switch (type) {
#define CREATE_LOG_SWITCH_CASE(name, prefix, stream, debug_only, use_log_class) case log_type::name: return prefix;
            LOG_INTERFACES(CREATE_LOG_SWITCH_CASE)
#undef CREATE_LOG_SWITCH_CASE
            default: return "LOG";
        }
    }

    namespace _impl {
        template <std::ostream *strm>
        class stream_template {
        public:
            constexpr static std::ostream *native = strm;
        };

        /*
            Variadic template functions for stream output.
        */
        template <std::ostream *strm>
        class base_stream : public stream_template<strm> {
        private:
            base_stream() = delete;
            ~base_stream() = delete;

        public:
            inline static void log(void) {
                *strm << std::endl;
            }

            template <typename T>
            static void log(const T &t) {
                *strm << t << std::endl;
            }

            template <typename T, typename... Ts>
            static void log(const T &t, const Ts &... ts) {
                *strm << t << ' ';
                log(ts...);
            }

            inline static void stream(void) {
                strm->flush();
            }

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
            Prefixes all messages with timestamp and type of message.
        */
        template <std::ostream *strm, log_type type>
        class log_stream : public stream_template<strm> {
        private:
            log_stream() = delete;
            ~log_stream() = delete;

            static void print_prefix() {
                auto now = boost::posix_time::second_clock::local_time();
                *strm << '[' << boost::posix_time::to_simple_string(now) << "] " << get_log_prefix(type) << " --- ";
            }

        public:
            template <typename... Ts>
            static void log(const Ts &... ts) {
                print_prefix();
                base_stream<strm>::log(ts...);
            }

            template <typename... Ts>
            static void stream(const Ts &... ts) {
                print_prefix();
                base_stream<strm>::stream(ts...);
            }

            template <typename... Ts>
            static void stream(const manip_t &manip, const Ts &... ts) {
                print_prefix();
                base_stream<strm>::stream(manip, ts...);
            }
        };

        /*
            Uses stream-specific mutex to safely output data across multiple threads.
        */
        template <std::ostream *strm>
        class mutex_guarded_base_stream : public stream_template<strm> {
        private:
            mutex_guarded_base_stream() = delete;
            ~mutex_guarded_base_stream() = delete;

        public:
            template <typename... Ts>
            static void log(const Ts &... ts) {
                auto &&lock = logging_mutex<strm>::acquire_lock_guard();
                base_stream<strm>::log(ts...);
            }

            template <typename... Ts>
            static void stream(const Ts &... ts) {
                auto &&lock = logging_mutex<strm>::acquire_lock_guard();
                base_stream<strm>::stream(ts...);
            }

            template <typename... Ts>
            static void stream(const manip_t &manip, const Ts &... ts) {
                auto &&lock = logging_mutex<strm>::acquire_lock_guard();
                base_stream<strm>::stream(manip, ts...);
            }
        };

        /*
            Uses stream-specific mutex to safely log data across multiple threads.
        */
        template <std::ostream *strm, log_type type>
        class mutex_guarded_log_stream : public stream_template<strm> {
        private:
            mutex_guarded_log_stream() = delete;
            ~mutex_guarded_log_stream() = delete;

        public:
            template <typename... Ts>
            static void log(const Ts &... ts) {
                auto &&lock = logging_mutex<strm>::acquire_lock_guard();
                log_stream<strm, type>::log(ts...);
            }

            template <typename... Ts>
            static void stream(const Ts &... ts) {
                auto &&lock = logging_mutex<strm>::acquire_lock_guard();
                log_stream<strm, type>::stream(ts...);
            }

            template <typename... Ts>
            static void stream(const manip_t &manip, const Ts &... ts) {
                auto &&lock = logging_mutex<strm>::acquire_lock_guard();
                log_stream<strm, type>::stream(manip, ts...);
            }
        };

        /*
            Null operations that look like stream functions.
        */
        class nullopt_stream {
        private:
            nullopt_stream() = delete;
            ~nullopt_stream() = delete;

        public:
            template <typename... Ts>
            constexpr static void log(const Ts &... ts) { }

            template <typename... Ts>
            constexpr static void stream(const Ts &... ts) { }

            template <typename... Ts>
            constexpr static void stream(const manip_t &manip, const Ts &... ts) { }
        };
    }

    // Define class for each log interface

#ifndef NDEBUG

#define CREATE_LOG_CLASSES_true(name, prefix, stream) \
using name = _impl::log_stream<&##stream, log_type::name>; \
using safe_##name = _impl::mutex_guarded_log_stream<&##stream, log_type::name>;

#define CREATE_LOG_CLASSES_false(name, prefix, stream) \
using name = _impl::base_stream<&##stream>; \
using safe_##name = _impl::mutex_guarded_base_stream<&##stream>;

#define CREATE_LOG_CLASSES(name, prefix, stream, debug_only, use_log_class) \
CREATE_LOG_CLASSES_##use_log_class(name, prefix, stream)

#else   // NDEBUG

#define CREATE_LOG_CLASSES_true_false(name, prefix, stream) \
using name = _impl::nullopt_stream; \
using safe_##name = _impl::nullopt_stream;

#define CREATE_LOG_CLASSES_true_true(name, prefix, stream) \
using name = _impl::nullopt_stream; \
using safe_##name = _impl::nullopt_stream;

#define CREATE_LOG_CLASSES_false_true(name, prefix, stream) \
using name = _impl::log_stream<&##stream, log_type::name>; \
using safe_##name = _impl::mutex_guarded_log_stream<&##stream, log_type::name>;

#define CREATE_LOG_CLASSES_false_false(name, prefix, stream) \
using name = _impl::base_stream<&##stream>; \
using safe_##name = _impl::mutex_guarded_base_stream<&##stream>;

#define CREATE_LOG_CLASSES(name, prefix, stream, debug_only, use_log_class) \
CREATE_LOG_CLASSES_##debug_only##_##use_log_class(name, prefix, stream)

#endif  // NDEBUG

    LOG_INTERFACES(CREATE_LOG_CLASSES)

#ifndef NDEBUG
#undef CREATE_LOG_CLASSES
#undef CREATE_LOG_CLASSES_false
#undef CREATE_LOG_CLASSES_true
#else
#undef CREATE_LOG_CLASSES
#undef CREATE_LOG_CLASSES_false_false
#undef CREATE_LOG_CLASSES_false_true
#undef CREATE_LOG_CLASSES_true_true
#undef CREATE_LOG_CLASSES_true_false
#endif

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