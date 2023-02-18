/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "acceptor.hpp"

#include <boost/lexical_cast.hpp>
#include <functional>
#include <memory>

#include "aether/proxy/error/error.hpp"
#include "aether/util/console.hpp"
#include "aether/util/result.hpp"
#include "aether/util/result_macros.hpp"

namespace proxy {

result<std::unique_ptr<acceptor>> acceptor::create(server_components& components) {
  std::unique_ptr<acceptor> acc(new acceptor(components));
  RETURN_IF_ERROR(acc->initialize());
  return acc;
}

acceptor::acceptor(server_components& components)
    : options_(components.options),
      io_contexts_(components.io_contexts),
      connection_manager_(components.connection_manager),
      endpoint_(options_.ipv6 ? boost::asio::ip::tcp::v6() : boost::asio::ip::tcp::v4(), options_.port),
      acc_(io_contexts_.get_io_context(), endpoint_),
      is_stopped_(false) {}

result<void> acceptor::initialize() {
  boost::system::error_code ec;
  if (options_.ipv6) {
    acc_.set_option(boost::asio::ip::v6_only(false), ec);
    acc_.set_option(boost::asio::socket_base::send_buffer_size(64 * 1024));
    if (ec != boost::system::errc::success) {
      return error::ipv6_error(out::string::stream("Could not configure dual stack socket (error code = ", ec.value(),
                                                   "). Use --ipv6=false to disable IPv6."));
    }
  }
  acc_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
  if (ec != boost::system::errc::success) {
    return error::acceptor_error("Could not configure socket option SO_REUSEADDR.");
  }
  return util::ok;
}

void acceptor::start() {
  acc_.listen(options_.connection_queue_limit);
  init_accept();
}

void acceptor::stop() { is_stopped_.store(true); }

void acceptor::init_accept() {
  auto& new_connection = connection_manager_.new_connection(io_contexts_.get_io_context());
  acc_.async_accept(new_connection.client.socket(), [this, &new_connection](const boost::system::error_code& error) {
    if (result<void> res = on_accept(new_connection, error); res.is_err()) {
      out::safe_error::log(res);
    }
  });
}

result<void> acceptor::on_accept(connection::connection_flow& connection, const boost::system::error_code& error) {
  if (error != boost::system::errc::success) {
    connection_manager_.destroy(connection);
    init_accept();
    return error::acceptor_error(out::string::stream(error.message(), " (", error.to_string(), ')'));
  }
  connection.client.set_connected();
  connection_manager_.start(connection);

  if (!is_stopped_.load()) {
    init_accept();
  } else {
    acc_.close();
  }

  return util::ok;
}

}  // namespace proxy
