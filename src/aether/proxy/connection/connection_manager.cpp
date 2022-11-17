/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "connection_manager.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <utility>

#include "aether/proxy/server_components.hpp"

namespace proxy::connection {

connection_manager::connection_manager(server_components& components) : components_(components) {}

connection_flow& connection_manager::new_connection(boost::asio::io_context& ioc) {
  auto ptr = std::make_unique<connection_flow>(ioc, components_);
  std::lock_guard<std::mutex> lock(data_mutex_);
  auto res = connections_.emplace(ptr->id(), std::move(ptr));
  return *res.first->second;
}

void connection_manager::start_service(connection_flow& flow) {
  auto res = services_.emplace(std::make_unique<connection_handler>(flow, components_));
  connection_handler& new_handler = *res.first->get();
  new_handler.start(std::bind_front(&connection_manager::stop, this, std::cref(*res.first)));
}

void connection_manager::start(connection_flow& flow) {
  std::lock_guard<std::mutex> lock(data_mutex_);

  // TODO: Some way to control how many connections we are servicing at a time.
  start_service(flow);
}

void connection_manager::destroy(connection_flow& flow) {
  std::lock_guard<std::mutex> lock(data_mutex_);
  connections_.erase(flow.id());
}

void connection_manager::stop(const std::unique_ptr<connection_handler>& service_ptr) {
  connection_flow& flow = service_ptr->connection_flow();
  std::lock_guard<std::mutex> lock(data_mutex_);
  services_.erase(service_ptr);
  // TODO: Make sure connection is safe for deletion?
  connections_.erase(flow.id());
}

void connection_manager::stop_all() {
  std::lock_guard<std::mutex> lock(data_mutex_);

  for (auto& current_service : services_) {
    current_service->stop();
  }
}

}  // namespace proxy::connection
