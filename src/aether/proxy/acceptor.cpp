/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "acceptor.hpp"

#include <boost/lexical_cast.hpp>
#include <functional>

#include "aether/proxy/error/exceptions.hpp"
#include "aether/util/console.hpp"

namespace proxy {

acceptor::acceptor(server_components& components)
    : options_(components.options),
      io_contexts_(components.io_contexts),
      connection_manager_(components.connection_manager),
      endpoint_(options_.ipv6 ? boost::asio::ip::tcp::v6() : boost::asio::ip::tcp::v4(), options_.port),
      acc_(io_contexts_.get_io_context(), endpoint_),
      is_stopped_(false) {
  boost::system::error_code ec;
  if (options_.ipv6) {
    acc_.set_option(boost::asio::ip::v6_only(false), ec);
    acc_.set_option(boost::asio::socket_base::send_buffer_size(64 * 1024));
    if (ec != boost::system::errc::success) {
      throw error::ipv6_error_exception(out::string::stream(
          "Could not configure dual stack socket (error code = ", ec.value(), "). Use --ipv6=false to disable IPv6."));
    }
  }
  acc_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
  if (ec != boost::system::errc::success) {
    throw error::acceptor_error_exception("Could not configure socket option SO_REUSEADDR.");
  }
}

void acceptor::start() {
  acc_.listen(options_.connection_queue_limit);
  init_accept();
}

void acceptor::stop() { is_stopped_.store(true); }

void acceptor::init_accept() {
  auto& new_connection = connection_manager_.new_connection(io_contexts_.get_io_context());
  acc_.async_accept(new_connection.client.socket(), [this, &new_connection](const boost::system::error_code& error) {
    on_accept(new_connection, error);
  });
}

void acceptor::on_accept(connection::connection_flow& connection, const boost::system::error_code& error) {
  if (error != boost::system::errc::success) {
    connection_manager_.destroy(connection);
    init_accept();
    throw error::acceptor_error_exception(out::string::stream(error.message(), " (", error, ')'));
  }

  connection_manager_.start(connection);

  if (!is_stopped_.load()) {
    init_accept();
  } else {
    acc_.close();
  }
}

}  // namespace proxy
