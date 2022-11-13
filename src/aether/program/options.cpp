/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "options.hpp"

#include <stdlib.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <string>
#include <thread>
#include <utility>

#include "aether/proxy/tls/openssl/ssl_method.hpp"
#include "aether/proxy/tls/x509/client_store.hpp"
#include "aether/proxy/tls/x509/server_store.hpp"
#include "aether/proxy/types.hpp"
#include "aether/util/console.hpp"
#include "aether/util/validate.hpp"

namespace program {

// Prints the help message to out::console.
void options::print_help() const {
  out::raw_stdout::log(
      "Aether is a simple HTTP/HTTPS/WebSocket proxy server written in C++ using Boost.Asio and OpenSSL.");
  out::raw_stdout::log("Usage:", command_name_, usage_);
  out::raw_stdout::log();
  parser_.print_options();
  out::raw_stdout::log();
}

void options::add_options() {
  parser_.add_option(command_line_option<proxy::port_t>{
      "port",
      'p',
      &port,
      false,
      3000,
      "Specifies the port to listen on.",
  });

  parser_.add_option(command_line_option<bool>{
      "help",
      'h',
      &help,
      false,
      false,
      "Displays help and options.",
  });

  parser_.add_option(command_line_option<bool>{
      "ipv6",
      '6',
      &ipv6,
      false,
      true,
      "Enables IPv6 using a dual stack socket.",
  });

  parser_.add_option(command_line_option<int>{
      "threads",
      std::nullopt,
      &thread_pool_size,
      false,
      util::validate::resolve_default_value<int>([](auto t) { return t > 0; }, std::thread::hardware_concurrency() * 2,
                                                 2),
      "Number of threads for the server to run.",
      [](auto t) { return t > 0; },
  });

  // TODO: max_listen_connections not linking.
  parser_.add_option(command_line_option<int>{
      "connection-limit",
      std::nullopt,
      &connection_queue_limit,
      false,
      SOMAXCONN,  // boost::asio::socket_base::max_listen_connections,
      "Number of connections that can be queued for service at one time.",
      [](auto q) { return q > 0; },
  });

  parser_.add_option(command_line_option<std::size_t, proxy::milliseconds>{
      "timeout",
      std::nullopt,
      &timeout,
      false,
      120000,
      "Milliseconds for connect, read, and write operations to timeout.",
      [](auto t) { return t != 0; },
      [](auto t) { return proxy::milliseconds(t); },
  });

  parser_.add_option(command_line_option<std::size_t, proxy::milliseconds>{
      "tunnel-timeout",
      std::nullopt,
      &tunnel_timeout,
      false,
      30000,
      "Milliseconds for tunnel operations to timeout.",
      [](auto t) { return t != 0; },
      [](auto t) { return proxy::milliseconds(t); },
  });

  parser_.add_option(command_line_option<std::size_t>{
      "body-size-limit",
      std::nullopt,
      &body_size_limit,
      false,
      200'000'000,
      "Maximum body size (in bytes) to allow through the proxy. Must be greater than 4096.",
      [](auto l) { return l > 4096; },
  });

  parser_.add_option(command_line_option<bool>{
      "ssl-passthrough-strict",
      std::nullopt,
      &ssl_passthrough_strict,
      false,
      false,
      "Passes all CONNECT requests to a TCP tunnel and does not use TLS services.",
  });

  parser_.add_option(command_line_option<bool>{
      "ssl-passthrough",
      std::nullopt,
      &ssl_passthrough,
      false,
      false,
      "Passes all CONNECT requests to a TCP tunnel unless explicitly marked for SSL interception.",
  });

  parser_.add_option(command_line_option<std::string, boost::asio::ssl::context::method>{
      "ssl-client-method",
      std::nullopt,
      &ssl_client_method,
      false,
      boost::lexical_cast<std::string>(boost::asio::ssl::context::method::sslv23),
      "SSL method to be used by the client when connecting to the proxy.",
      &util::validate::lexical_castable<std::string, boost::asio::ssl::context::method>,
      [](const std::string& m) { return boost::lexical_cast<boost::asio::ssl::context::method>(m); },
  });

  parser_.add_option(command_line_option<std::string, boost::asio::ssl::context::method>{
      "ssl-server-method",
      std::nullopt,
      &ssl_server_method,
      false,
      boost::lexical_cast<std::string>(boost::asio::ssl::context::method::sslv23),
      "SSL method to be used by the server when connecting to an upstream server.",
      &util::validate::lexical_castable<std::string, boost::asio::ssl::context::method>,
      [](const std::string& m) { return boost::lexical_cast<boost::asio::ssl::context::method>(m); },
  });

  parser_.add_option(command_line_option<bool, int>{
      "ssl-verify",
      std::nullopt,
      &ssl_verify,
      false,
      true,
      "Verify the upstream server's SSL certificate.",
      std::nullopt,
      [](bool verify) {
        return verify ? boost::asio::ssl::context::verify_peer : boost::asio::ssl::context::verify_none;
      },
  });

  parser_.add_option(command_line_option<bool>{
      "ssl-negotiate-ciphers",
      std::nullopt,
      &ssl_negotiate_ciphers,
      false,
      false,
      "Negotiate the SSL cipher suites with the server, regardless of the options the client sends.",
  });

  parser_.add_option(command_line_option<bool>{
      "ssl-negotiate-alpn",
      std::nullopt,
      &ssl_negotiate_alpn,
      false,
      false,
      "Negotiate the ALPN protocol with the server, regardless of the options the client sends.",
  });

  parser_.add_option(command_line_option<bool>{
      "ssl-supply-server-chain",
      std::nullopt,
      &ssl_supply_server_chain_to_client,
      false,
      false,
      "Supply the upstream server's certificate chain to the proxy client.",
  });

  parser_.add_option(command_line_option<std::string>{
      "ssl-certificate-properties",
      std::nullopt,
      &ssl_cert_store_properties,
      false,
      proxy::tls::x509::server_store::default_properties_file.string(),
      "Path to a .properties file for the server's certificate configuration.",
      [](const std::string& path) { return boost::filesystem::exists(path); },
  });

  parser_.add_option(command_line_option<std::string>{
      "ssl-certificate-dir",
      std::nullopt,
      &ssl_cert_store_dir,
      false,
      proxy::tls::x509::server_store::default_dir.string(),
      "Folder containing the server's SSL certificates, or the destination folder for generated certificates.",
      [](const std::string& path) {
        return boost::filesystem::exists(path) ||
               boost::filesystem::exists(boost::filesystem::path(path).parent_path());
      },
  });

  parser_.add_option(command_line_option<std::string>{
      "ssl-dhparam-file",
      std::nullopt,
      &ssl_dhparam_file,
      false,
      proxy::tls::x509::server_store::default_dhparam_file.string(),
      "Path to a .pem file containing the server's Diffie-Hellman parameters.",
      [](const std::string& path) { return boost::filesystem::exists(path); },
  });

  parser_.add_option(command_line_option<std::string>{
      "upstream-trusted-ca-file",
      std::nullopt,
      &ssl_verify_upstream_trusted_ca_file_path,
      false,
      proxy::tls::x509::client_store::default_trusted_certificates_file.string(),
      "Path to a PEM-formatted trusted CA certificate for upstream verification.",
      [](const std::string& path) { return boost::filesystem::exists(path); },
  });

  parser_.add_option(command_line_option<bool>{
      "ws-passthrough-strict",
      std::nullopt,
      &websocket_passthrough_strict,
      false,
      false,
      "Passes all WebSocket connections to a TCP tunnel and does not use WebSocket services.",
  });

  parser_.add_option(command_line_option<bool>{
      "ws-passthrough",
      std::nullopt,
      &websocket_passthrough,
      false,
      false,
      "Passes all WebSocket connections to a TCP tunnel unless explicitly marked for interception.",
  });

  parser_.add_option(command_line_option<bool>{
      "ws-intercept-default",
      std::nullopt,
      &websocket_intercept_messages_by_default,
      false,
      false,
      "Intercept all WebSocket messages by default.",
  });

  parser_.add_option(command_line_option<bool>{
      "interactive",
      std::nullopt,
      &run_interactive,
      false,
      false,
      "Runs a command-line service for interacting with the server in real time.",
  });
  parser_.add_option(command_line_option<bool>{
      "logs",
      std::nullopt,
      &run_logs,
      false,
      false,
      "Logs all server activity to the console.",
  });
  parser_.add_option(command_line_option<bool, bool>{
      "silent",
      's',
      &run_silent,
      false,
      false,
      "Prints nothing to stdout while the server runs.",
  });

  parser_.add_option(command_line_option<std::string>{
      "log-file",
      'l',
      &log_file_name,
      false,
      std::nullopt,
      "Redirect all log output to given output file. Redirects stdout and stderr.",
      [](const std::string& path) { return std::ofstream(path).good(); },
  });
}

void options::parse_cmdline(int argc, char* argv[]) {
  // Don't use std::once_flag and std::call_once, since it's not copyable.
  if (!options_added_) {
    add_options();
    options_added_ = true;
  }

  command_name_ = argv[0];
  usage_ = "[OPTIONS]";

  try {
    parser_.parse(argc, argv);
  } catch (const option_exception& ex) {
    out::raw_stderr::log(ex.what());
    out::raw_stderr::log("Usage: ", usage_);
    out::raw_stderr::stream("Use `", argv[0], " --help` to view options", out::manip::endl);
    exit(1);
  }

  // The help option ends the program.
  if (help) {
    print_help();
    std::exit(0);
  }
}

}  // namespace program
