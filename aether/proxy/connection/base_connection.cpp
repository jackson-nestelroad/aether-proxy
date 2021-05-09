/*********************************************

    Copyright (c) Jackson Nestelroad 2020
    jackson.nestelroad.com

*********************************************/

#include "base_connection.hpp"
#include <aether/proxy/server_components.hpp>

namespace proxy::connection {
    base_connection::base_connection(boost::asio::io_context &ioc, server_components &components)
        : options(components.options),
        ioc(ioc),
        // TODO: boost::asio::detail::win_mutex leak
        strand(boost::asio::make_strand(ioc)),
        socket(strand),
        timeout(ioc),
        mode(io_mode::regular),
        tls_established(false),
        secure_socket(),
        ssl_context(),
        cert(nullptr)
    { }

    base_connection::~base_connection() { }

    void base_connection::set_timeout() {
        switch (mode) {
            case io_mode::regular:
                timeout.set_timeout(options.timeout, boost::bind(&base_connection::on_timeout, this)); return;
            case io_mode::tunnel: 
                timeout.set_timeout(options.tunnel_timeout, boost::bind(&base_connection::on_timeout, this)); return;
            case io_mode::no_timeout: 
                return;
        }
    }

    bool base_connection::has_been_closed() {
        boost::system::error_code error;

        socket.non_blocking(true);
        socket.receive(input.prepare_sequence(1), boost::asio::ip::tcp::socket::message_peek, error);
        socket.non_blocking(false);

        if (error == boost::asio::error::eof) {
            return true;
        }
        return false;
    }

    void base_connection::set_mode(io_mode new_mode) {
        mode = new_mode;
    }

    base_connection::io_mode base_connection::get_mode() const {
        return mode;
    }

    bool base_connection::secured() const {
        return tls_established;
    }


    tcp::tls::x509::certificate base_connection::get_cert() const {
        return cert;
    }

    const std::string &base_connection::get_alpn() const {
        return alpn;
    }

    std::size_t base_connection::read(boost::system::error_code &error) {
        return read(default_buffer_size, error);
    }

    std::size_t base_connection::read(std::size_t buffer_size, boost::system::error_code &error) {
        set_timeout();
        
        std::size_t bytes_read = 0;
        if (tls_established) {
            bytes_read = secure_socket->read_some(input.prepare_sequence(buffer_size), error);
        }
        else {
            bytes_read = socket.read_some(input.prepare_sequence(buffer_size), error);
        }

        input.commit(bytes_read);
        timeout.cancel_timeout();
        return bytes_read;
    }
    
    std::size_t base_connection::read_available(boost::system::error_code &error) {
        socket.non_blocking(true);

        std::size_t bytes_read = 0;
        if (tls_established) {
            bytes_read = boost::asio::read(socket, util::buffer::asio_dynamic_buffer_v1(input), error);
        }
        else {
            bytes_read = boost::asio::read(*secure_socket, util::buffer::asio_dynamic_buffer_v1(input), error);
        }

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
        if (tls_established) {
            secure_socket->async_read_some(input.prepare_sequence(buffer_size), boost::asio::bind_executor(strand, 
                boost::bind(&base_connection::on_read_need_to_commit, shared_from_this(), handler,
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
        }
        else {
            socket.async_read_some(input.prepare_sequence(buffer_size), boost::asio::bind_executor(strand,
                boost::bind(&base_connection::on_read_need_to_commit, shared_from_this(), handler,
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
        }
    }

    void base_connection::read_until_async(std::string_view delim, const io_callback &handler) {
        set_timeout();
        if (tls_established) {
            boost::asio::async_read_until(*secure_socket, util::buffer::asio_dynamic_buffer_v1(input), delim, boost::asio::bind_executor(strand,
                boost::bind(&base_connection::on_read, shared_from_this(), handler,
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
        }
        else {
            boost::asio::async_read_until(socket, util::buffer::asio_dynamic_buffer_v1(input), delim, boost::asio::bind_executor(strand,
                boost::bind(&base_connection::on_read, shared_from_this(), handler,
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
        }
    }

    void base_connection::on_read(const io_callback &handler, const boost::system::error_code &error, std::size_t bytes_transferred) {
        timeout.cancel_timeout();
        boost::asio::post(ioc, boost::bind(handler, error, bytes_transferred));
    }

    void base_connection::on_read_need_to_commit(const io_callback &handler, const boost::system::error_code &error, std::size_t bytes_transferred) {
        input.commit(bytes_transferred);
        on_read(handler, error, bytes_transferred);
    }

    std::size_t base_connection::write(boost::system::error_code &error) {
        set_timeout();

        std::size_t bytes_written = 0;
        if (tls_established) {
            bytes_written = boost::asio::write(*secure_socket, util::buffer::asio_dynamic_buffer_v1(output), error);
        }
        else {
            bytes_written = boost::asio::write(socket, util::buffer::asio_dynamic_buffer_v1(output), error);
        }

        timeout.cancel_timeout();
        return bytes_written;
    }

    void base_connection::write_async(const io_callback &handler) {
        set_timeout();
        if (tls_established) {
            boost::asio::async_write(*secure_socket, util::buffer::asio_dynamic_buffer_v1(output), boost::asio::bind_executor(strand,
                boost::bind(&base_connection::on_write, shared_from_this(), handler,
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
        }
        else {
            boost::asio::async_write(socket, util::buffer::asio_dynamic_buffer_v1(output), boost::asio::bind_executor(strand,
                boost::bind(&base_connection::on_write, shared_from_this(), handler,
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
        }
    }

    void base_connection::write_untimed_async(const io_callback &handler) {
        if (tls_established) {
            boost::asio::async_write(*secure_socket, util::buffer::asio_dynamic_buffer_v1(output), boost::asio::bind_executor(strand,
                boost::bind(&base_connection::on_untimed_write, shared_from_this(), handler,
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
        }
        else {
            boost::asio::async_write(socket, util::buffer::asio_dynamic_buffer_v1(output), boost::asio::bind_executor(strand,
            boost::bind(&base_connection::on_untimed_write, shared_from_this(), handler,
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
        }
    }

    void base_connection::on_write(const io_callback &handler, const boost::system::error_code &error, std::size_t bytes_transferred) {
        timeout.cancel_timeout();
        boost::asio::post(ioc, boost::bind(handler, error, bytes_transferred));
    }

    void base_connection::on_untimed_write(const io_callback &handler, const boost::system::error_code &error, std::size_t bytes_transferred) {
        boost::asio::post(ioc, boost::bind(handler, error, bytes_transferred));
    }

    void base_connection::shutdown() {
        socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    }

    void base_connection::on_timeout() {
        // TODO: Maybe ignore error here altogether? Timeout usually means we're finished anyway
        boost::system::error_code error;
        socket.cancel(error);
        switch (error.value()) {
            case boost::system::errc::success:
            case boost::system::errc::no_such_device_or_address:
            case boost::asio::error::bad_descriptor:
                break;
            default:
                throw error::asio_error_exception { error.message() };
        }
    }

    void base_connection::cancel(boost::system::error_code &error) {
        socket.cancel(error);
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

    boost::asio::io_context &base_connection::io_context() {
        return ioc;
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

    streambuf &base_connection::input_buffer() {
        return input;
    }

    streambuf &base_connection::output_buffer() {
        return output;
    }

    const_buffer base_connection::const_input_buffer() const {
        return input.data();
    }

    boost::asio::ip::tcp::endpoint base_connection::get_endpoint() const {
        return socket.remote_endpoint();
    }

    boost::asio::ip::address base_connection::get_address() const {
        return socket.remote_endpoint().address();
    }

    base_connection &base_connection::operator<<(const byte_array &data) {
        std::copy(data.begin(), data.end(), std::ostreambuf_iterator<char>(&output));
        return *this;
    }
}