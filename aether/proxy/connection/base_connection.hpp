/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#pragma once

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

#include <aether/proxy/types.hpp>
#include <aether/proxy/connection/timeout_service.hpp>
#include <aether/util/console.hpp>

namespace proxy::connection {
    /*
        Base class for a TCP socket connection.
        Can be thought of as a wrapper around the socket class.
    */
    class base_connection
        // shared_from_this() is used in async handlers to assure the connection stays alive until all handlers are finished
        : public std::enable_shared_from_this<base_connection>,
        private boost::noncopyable {
    public:
        static constexpr std::size_t default_timeout_ms = 500000;
        static constexpr std::size_t default_tunnel_timeout_ms = 30000;
        static constexpr std::size_t default_buffer_size = 8192;

        using ptr = std::shared_ptr<base_connection>;

        /*
            Enum to represent operation mode.
            Changes the timeout for socket operations.
        */
        enum class io_mode {
            regular,
            tunnel,
            no_timeout
        };

    protected:
        static milliseconds default_timeout;
        static milliseconds default_tunnel_timeout;

        boost::asio::io_service &ios;
        boost::asio::ip::tcp::socket socket;
        timeout_service timeout;
        streambuf input;
        streambuf output;
        io_mode mode;

        base_connection(boost::asio::io_service &ios);

        /*
            Sends the shutdown signal over the socket.
        */
        void shutdown();

        /*
            Cancels the current socket connection.
        */
        void cancel();

        /*
            Turns on the timeout service to cancel the socket after timeout_ms.
            Must call timeout.cancel_timeout() to stop the timer.
            Calls the handler with a timeout error code if applicable.
        */
        void set_timeout();

        /*
            Callback for read_async.
            Use when commit is called automatically.
        */
        void on_read(const io_callback &handler, const boost::system::error_code &error, std::size_t bytes_transferred);

        /*
            Callback for read_async.
            Commits the bytes_transferred to the buffer.
        */
        void on_read_need_to_commit(const io_callback &handler, const boost::system::error_code &error, std::size_t bytes_transferred);

        /*
            Callback for write_async.
        */
        void on_write(const io_callback &handler, const boost::system::error_code &error, std::size_t bytes_transferred);

        /*
            Callback for write_untimed_async.
        */
        void on_untimed_write(const io_callback &handler, const boost::system::error_code &error, std::size_t bytes_transferred);

    public:
        /*
            Sets the timeout duration for all connections.
        */
        static void set_timeout_duration(std::size_t ms);

        /*
            Sets the tunnel timeout duration for all connections.
        */
        static void set_tunnel_timeout_duration(std::size_t ms);

        void set_mode(io_mode new_mode);

        /*
            Tests if the socket has been closed.
            Reads produce EOF error.
        */
        bool has_been_closed();

        /*
            Reads from the socket synchronously with a custom buffer size.
            Calls socket.read_some.
        */
        std::size_t read(std::size_t buffer_size, boost::system::error_code &error);

        /*
            Reads from the socket synchronously.
            Calls socket.read_some.
        */
        std::size_t read(boost::system::error_code &error);

        /*
            Reads all bytes currently available in the socket.
            This operation must be non-blocking.
            Calls boost::asio::read.
        */
        std::size_t read_available(boost::system::error_code &error);

        /*
            Reads from the socket asynchronously with a custom buffer size.
            Calls socket.async_read_some.
        */
        void read_async(std::size_t buffer_size, const io_callback &handler);

        /*
            Reads from the socket asynchronously.
            Calls socket.async_read_some.
        */
        void read_async(const io_callback &handler);

        /*
            Reads from the socket asynchronously until the given delimiter is in the buffer.
            Calls boost::asio::async_read_until.
        */
        void read_until_async(std::string_view delim, const io_callback &handler);

        /*
            Writes to the socket synchronously.
            This operation must be non-blocking.
        */
        std::size_t write(boost::system::error_code &error);

        /*
            Writes to the socket asynchronously using the output buffer.
        */
        void write_async(const io_callback &handler);

        /*
            Writes to the socket asynchronously using the output buffer.
            Does not put a timeout on the operation.
            Only use when a timeout is placed on another concurrent operation.
        */
        void write_untimed_async(const io_callback &handler);
        
        /*
            Closes the socket.
        */
        void close();

        boost::asio::ip::tcp::socket &get_socket();
        boost::asio::io_service &io_service();

        /*
            Returns the number of bytes that are available to be read without blocking.
        */
        std::size_t available_bytes() const;

        /*
            Returns an input stream for reading from the input buffer.
            The input buffer should first be read using connection.read_async.
        */
        std::istream input_stream();

        /*
            Returns an output stream for writing to the output buffer.
            The output buffer should then be written to the socket using connection.write_async.
        */
        std::ostream output_stream();

        boost::asio::ip::tcp::endpoint get_endpoint();
        boost::asio::ip::address get_address();

        template <typename T>
        base_connection &operator<<(const T &data) {
            std::ostream(&output) << data;
            return *this;
        }
    };
}