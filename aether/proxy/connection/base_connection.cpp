/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "base_connection.hpp"
#include "connection_manager.hpp"

namespace proxy::connection {
    milliseconds base_connection::default_timeout(default_timeout_ms);
    milliseconds base_connection::default_tunnel_timeout(default_tunnel_timeout_ms);

    base_connection::base_connection(boost::asio::io_service &ios)
        : ios(ios),
        socket(ios),
        timeout(ios),
        mode(io_mode::regular)
    { }

    void base_connection::set_timeout_duration(std::size_t ms) {
        default_timeout = milliseconds(ms);
    }

    void base_connection::set_tunnel_timeout_duration(std::size_t ms) {
        default_tunnel_timeout = milliseconds(ms);
    }

    void base_connection::set_timeout() {
        switch (mode) {
            case io_mode::regular: timeout.set_timeout(default_timeout, boost::bind(&base_connection::cancel, this)); return;
            case io_mode::tunnel: timeout.set_timeout(default_tunnel_timeout, boost::bind(&base_connection::cancel, this)); return;
            case io_mode::no_timeout: return;
        }
    }

    bool base_connection::has_been_closed() {
        socket.non_blocking(true);
        boost::system::error_code error;
        socket.receive(input.prepare(1), boost::asio::ip::tcp::socket::message_peek, error);
        socket.non_blocking(false);
        if (error == boost::asio::error::eof) {
            return true;
        }
        return false;
    }

    void base_connection::set_mode(io_mode new_mode) {
        mode = new_mode;
    }

    std::size_t base_connection::read(boost::system::error_code &error) {
        return read(default_buffer_size, error);
    }

    std::size_t base_connection::read(std::size_t buffer_size, boost::system::error_code &error) {
        set_timeout();
        std::size_t bytes_read = socket.read_some(input.prepare(buffer_size), error);
        input.commit(bytes_read);
        timeout.cancel_timeout();
        return bytes_read;
    }
    
    std::size_t base_connection::read_available(boost::system::error_code &error) {
        socket.non_blocking(true);
        std::size_t bytes_read = boost::asio::read(socket, input, error);
        if (error == boost::asio::error::would_block) {
            error = boost::system::errc::make_error_code(boost::system::errc::success);
        }
        socket.non_blocking(false);
        return bytes_read;
    }

    void base_connection::read_async(const io_callback &handler) {
        read_async(default_buffer_size, handler);
    }

    void base_connection::read_async(std::size_t buffer_size, const io_callback &handler) {
        set_timeout();
        socket.async_read_some(input.prepare(buffer_size),
            boost::bind(&base_connection::on_read_need_to_commit, shared_from_this(), handler,
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void base_connection::read_until_async(std::string_view delim, const io_callback &handler) {
        set_timeout();
        boost::asio::async_read_until(socket, input, delim,
            boost::bind(&base_connection::on_read, shared_from_this(), handler,
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void base_connection::on_read(const io_callback &handler, const boost::system::error_code &error, std::size_t bytes_transferred) {
        timeout.cancel_timeout();
        ios.post(boost::bind(handler, error, bytes_transferred));
    }

    void base_connection::on_read_need_to_commit(const io_callback &handler, const boost::system::error_code &error, std::size_t bytes_transferred) {
        input.commit(bytes_transferred);
        on_read(handler, error, bytes_transferred);
    }

    std::size_t base_connection::write(boost::system::error_code &error) {
        set_timeout();
        std::size_t bytes_written = boost::asio::write(socket, output, error);
        timeout.cancel_timeout();
        return bytes_written;
    }

    void base_connection::write_async(const io_callback &handler) {
        set_timeout();
        boost::asio::async_write(socket, output,
            boost::bind(&base_connection::on_write, shared_from_this(), handler,
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void base_connection::write_untimed_async(const io_callback &handler) {
        boost::asio::async_write(socket, output,
            boost::bind(&base_connection::on_untimed_write, shared_from_this(), handler,
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void base_connection::on_write(const io_callback &handler, const boost::system::error_code &error, std::size_t bytes_transferred) {
        timeout.cancel_timeout();
        ios.post(boost::bind(handler, error, bytes_transferred));
    }

    void base_connection::on_untimed_write(const io_callback &handler, const boost::system::error_code &error, std::size_t bytes_transferred) {
        ios.post(boost::bind(handler, error, bytes_transferred));
    }

    void base_connection::shutdown() {
        socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    }

    void base_connection::cancel() {
        boost::system::error_code error;
        socket.cancel(error);
        switch (error.value()) {
            case boost::system::errc::success:
            case boost::asio::error::bad_descriptor:
                break;
            default:
                out::error::log(error.message());
        }
    }

    void base_connection::close() {
        // shutdown();
        // Cancel any pending timeouts
        timeout.cancel_timeout();
        socket.close();
    }

    boost::asio::ip::tcp::socket &base_connection::get_socket() {
        return socket;
    }

    boost::asio::io_service &base_connection::io_service() {
        return ios;
    }

    std::size_t base_connection::available_bytes() const {
        return socket.available();
    }

    std::istream base_connection::input_stream() {
        return std::istream(&input);
    }

    std::ostream base_connection::output_stream() {
        return std::ostream(&output);
    }

    const_streambuf base_connection::const_input_buffer() const {
        return input.data();
    }

    boost::asio::ip::tcp::endpoint base_connection::get_endpoint() {
        return socket.remote_endpoint();
    }

    boost::asio::ip::address base_connection::get_address() {
        return socket.remote_endpoint().address();
    }

    base_connection &base_connection::operator<<(const byte_array &data) {
        auto out = std::ostream(&output);
        std::copy(data.begin(), data.end(), std::ostream_iterator<unsigned char>(out));
        return *this;
    }
}