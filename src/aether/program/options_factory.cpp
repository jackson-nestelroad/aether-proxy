/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "options_factory.hpp"

#include <stdlib.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <thread>
#include <utility>

#include "aether/proxy/tls/openssl/ssl_method.hpp"
#include "aether/proxy/tls/x509/client_store.hpp"
#include "aether/proxy/tls/x509/server_store.hpp"
#include "aether/proxy/types.hpp"
#include "aether/util/console.hpp"
#include "aether/util/generic_error.hpp"
#include "aether/util/result.hpp"
#include "aether/util/result_macros.hpp"
#include "aether/util/validate.hpp"

namespace program {

// Prints the help message to out::console.
void options_factory::print_help() const {
  out::raw_stdout::log(
      "Aether is a simple HTTP/HTTPS/WebSocket proxy server written in C++ using Boost.Asio and OpenSSL.");
  out::raw_stdout::log("Usage:", command_name_, usage_);
  out::raw_stdout::log();
  parser_.print_options();
  out::raw_stdout::log();
}

util::result<void, util::generic_error> options_factory::add_options() {
  RETURN_IF_ERROR(parser_.add_option(command_line_option<proxy::port_t>{
      .name = "port",
      .letter = 'p',
      .destination = &options_.port,
      .required = false,
      .default_value = 3000,
      .description = "Specifies the port to listen on.",
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<bool>{
      .name = "help",
      .letter = 'h',
      .destination = &options_.help,
      .required = false,
      .default_value = false,
      .description = "Displays help and options.",
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<bool>{
      .name = "ipv6",
      .letter = '6',
      .destination = &options_.ipv6,
      .required = false,
      .default_value = true,
      .description = "Enables IPv6 using a dual stack socket.",
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<int>{
      .name = "threads",
      .destination = &options_.thread_pool_size,
      .required = false,
      .default_value = util::validate::resolve_default_value<int>([](auto t) { return t > 0; },
                                                                  std::thread::hardware_concurrency() * 2, 2),
      .description = "Number of threads for the server to run.",
      .validate = [](auto t) { return t > 0; },
  }));

  // TODO: max_listen_connections not linking.
  RETURN_IF_ERROR(parser_.add_option(command_line_option<int>{
      .name = "connection-limit",
      .destination = &options_.connection_queue_limit,
      .required = false,
      .default_value = SOMAXCONN,  // boost::asio::socket_base::max_listen_connections,
      .description = "Number of connections that can be queued for server acceptor port at any given time.",
      .validate = [](auto q) { return q > 0; },
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<std::size_t>{
      .name = "connection-service-limit",
      .destination = &options_.connection_service_limit,
      .required = false,
      .default_value = std::numeric_limits<std::size_t>::max(),
      .description = "Number of connections that can be serviced by the proxy at any given time.",
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<std::size_t, proxy::milliseconds>{
      .name = "timeout",
      .destination = &options_.timeout,
      .required = false,
      .default_value = 120000,
      .description = "Milliseconds for connect, read, and write operations to timeout.",
      .validate = [](auto t) { return t != 0; },
      .converter = [](auto t) { return proxy::milliseconds(t); },
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<std::size_t, proxy::milliseconds>{
      .name = "tunnel-timeout",
      .destination = &options_.tunnel_timeout,
      .required = false,
      .default_value = 30000,
      .description = "Milliseconds for tunnel operations to timeout.",
      .validate = [](auto t) { return t != 0; },
      .converter = [](auto t) { return proxy::milliseconds(t); },
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<std::size_t>{
      .name = "body-size-limit",
      .destination = &options_.body_size_limit,
      .required = false,
      .default_value = 200'000'000,
      .description = "Maximum body size (in bytes) to allow through the proxy. Must be greater than 4096.",
      .validate = [](auto l) { return l > 4096; },
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<bool>{
      .name = "ssl-passthrough-strict",
      .destination = &options_.ssl_passthrough_strict,
      .required = false,
      .default_value = false,
      .description = "Passes all CONNECT requests to a TCP tunnel and does not use TLS services.",
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<bool>{
      .name = "ssl-passthrough",
      .destination = &options_.ssl_passthrough,
      .required = false,
      .default_value = false,
      .description = "Passes all CONNECT requests to a TCP tunnel unless explicitly marked for SSL interception.",
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<std::string, boost::asio::ssl::context::method>{
      .name = "ssl-client-method",
      .destination = &options_.ssl_client_method,
      .required = false,
      .default_value =
          std::string(proxy::tls::openssl::ssl_method_to_string(boost::asio::ssl::context::method::sslv23).ok()),
      .description = "SSL method to be used by the client when connecting to the proxy.",
      .validate = [](const std::string& m) { return proxy::tls::openssl::string_to_ssl_method(m).is_ok(); },
      .converter = [](const std::string& m) { return proxy::tls::openssl::string_to_ssl_method(m).ok(); },
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<std::string, boost::asio::ssl::context::method>{
      .name = "ssl-server-method",
      .destination = &options_.ssl_server_method,
      .required = false,
      .default_value =
          std::string(proxy::tls::openssl::ssl_method_to_string(boost::asio::ssl::context::method::sslv23).ok()),
      .description = "SSL method to be used by the server when connecting to an upstream server.",
      .validate = [](const std::string& m) { return proxy::tls::openssl::string_to_ssl_method(m).is_ok(); },
      .converter = [](const std::string& m) { return proxy::tls::openssl::string_to_ssl_method(m).ok(); },
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<bool, int>{
      .name = "ssl-verify",
      .destination = &options_.ssl_verify,
      .required = false,
      .default_value = true,
      .description = "Verify the upstream server's SSL certificate.",
      .converter =
          [](bool verify) {
            return verify ? boost::asio::ssl::context::verify_peer : boost::asio::ssl::context::verify_none;
          },
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<bool>{
      .name = "ssl-negotiate-ciphers",
      .destination = &options_.ssl_negotiate_ciphers,
      .required = false,
      .default_value = false,
      .description = "Negotiate the SSL cipher suites with the server, regardless of the options the client sends.",
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<bool>{
      .name = "ssl-negotiate-alpn",
      .destination = &options_.ssl_negotiate_alpn,
      .required = false,
      .default_value = false,
      .description = "Negotiate the ALPN protocol with the server, regardless of the options the client sends.",
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<bool>{
      .name = "ssl-supply-server-chain",
      .destination = &options_.ssl_supply_server_chain_to_client,
      .required = false,
      .default_value = false,
      .description = "Supply the upstream server's certificate chain to the proxy client.",
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<std::string>{
      .name = "ssl-certificate-properties",
      .destination = &options_.ssl_cert_store_properties,
      .required = false,
      .default_value = proxy::tls::x509::server_store::default_properties_file.string(),
      .description = "Path to a .properties file for the server's certificate configuration.",
      .validate = [](const std::string& path) { return std::filesystem::exists(path); },
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<std::string>{
      .name = "ssl-certificate-dir",
      .destination = &options_.ssl_cert_store_dir,
      .required = false,
      .default_value = proxy::tls::x509::server_store::default_dir.string(),
      .description =
          "Folder containing the server's SSL certificates, or the destination folder for generated certificates.",
      .validate =
          [](const std::string& path) {
            return std::filesystem::exists(path) || std::filesystem::exists(std::filesystem::path(path).parent_path());
          },
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<std::string>{
      .name = "ssl-dhparam-file",
      .destination = &options_.ssl_dhparam_file,
      .required = false,
      .default_value = proxy::tls::x509::server_store::default_dhparam_file.string(),
      .description = "Path to a .pem file containing the server's Diffie-Hellman parameters.",
      .validate = [](const std::string& path) { return std::filesystem::exists(path); },
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<std::string>{
      .name = "upstream-trusted-ca-file",
      .destination = &options_.ssl_verify_upstream_trusted_ca_file_path,
      .required = false,
      .default_value = proxy::tls::x509::client_store::default_trusted_certificates_file.string(),
      .description = "Path to a PEM-formatted trusted CA certificate for upstream verification.",
      .validate = [](const std::string& path) { return std::filesystem::exists(path); },
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<bool>{
      .name = "strong-serial-numbers",
      .destination = &options_.ssl_use_strong_serial_numbers,
      .required = false,
      .default_value = false,
      .description = "Use strong serial numbers for generated certificates by checkpointing state to disk.",
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<bool>{
      .name = "ws-passthrough-strict",
      .destination = &options_.websocket_passthrough_strict,
      .required = false,
      .default_value = false,
      .description = "Passes all WebSocket connections to a TCP tunnel and does not use WebSocket services.",
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<bool>{
      .name = "ws-passthrough",
      .destination = &options_.websocket_passthrough,
      .required = false,
      .default_value = false,
      .description = "Passes all WebSocket connections to a TCP tunnel unless explicitly marked for interception.",
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<bool>{
      .name = "ws-intercept-default",
      .destination = &options_.websocket_intercept_messages_by_default,
      .required = false,
      .default_value = false,
      .description = "Intercept all WebSocket messages by default.",
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<bool>{
      .name = "interactive",
      .letter = 'i',
      .destination = &options_.run_interactive,
      .required = false,
      .default_value = false,
      .description = "Runs a command-line service for interacting with the server in real time.",
  }));
  RETURN_IF_ERROR(parser_.add_option(command_line_option<bool>{
      .name = "logs",
      .letter = 'l',
      .destination = &options_.run_logs,
      .required = false,
      .default_value = false,
      .description = "Logs all server activity to the console.",
  }));
  RETURN_IF_ERROR(parser_.add_option(command_line_option<bool, bool>{
      .name = "silent",
      .letter = 's',
      .destination = &options_.run_silent,
      .required = false,
      .default_value = false,
      .description = "Prints nothing to stdout while the server runs.",
  }));

  RETURN_IF_ERROR(parser_.add_option(command_line_option<std::string>{
      .name = "log-file",
      .destination = &options_.log_file_name,
      .required = false,
      .description = "Redirect all log output to given output file. Redirects stdout and stderr.",
      .validate = [](const std::string& path) { return std::ofstream(path).good(); },
  }));

  return util::ok;
}

void options_factory::parse_cmdline(int argc, char* argv[]) {
  // Don't use std::once_flag and std::call_once, since it's not copyable.
  if (!options_added_) {
    if (util::result<void, util::generic_error> result = add_options(); result.is_err()) {
      out::error::log("Failed to initialize command-line options, which is an error with the server binary");
    }
    options_added_ = true;
  }

  command_name_ = argv[0];
  usage_ = "[OPTIONS]";

  if (util::result<int, util::generic_error> result = parser_.parse(argc, argv); result.is_err()) {
    out::raw_stderr::log(result.err().message());
    out::raw_stderr::log("Usage: ", usage_);
    out::raw_stderr::stream("Use `", argv[0], " --help` to view options", out::manip::endl);
    std::exit(1);
  }

  // The help option ends the program.
  if (options_.help) {
    print_help();
    std::exit(0);
  }
}

}  // namespace program
