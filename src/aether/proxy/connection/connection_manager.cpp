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
  const std::unique_ptr<connection_handler>& service_ptr = *res.first;
  connection_handler& new_handler = *res.first->get();
  boost::asio::post(flow.io_context(), [this, &new_handler, service_ptr = std::cref(service_ptr)]() {
    new_handler.start(std::bind_front(&connection_manager::stop, this, std::cref(service_ptr)));
  });
}

void connection_manager::start(connection_flow& flow) {
  std::lock_guard<std::mutex> lock(data_mutex_);

  if (services_.size() < components_.options.connection_service_limit) {
    start_service(flow);
  } else {
    pending_connection_ids_.push(flow.id());
  }
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

  start_pending_connections();
}

void connection_manager::start_pending_connections() {
  while (!pending_connection_ids_.empty() && services_.size() < components_.options.connection_service_limit) {
    util::uuid_t next_connection_id = pending_connection_ids_.front();
    pending_connection_ids_.pop();
    auto next_connection_it = connections_.find(next_connection_id);
    if (next_connection_it == connections_.end()) {
      out::safe_warn::log("Connection id", next_connection_id,
                          "was in pending connection queue but does not reference any known connection");
      continue;
    }
    connection_flow& next_connection = *next_connection_it->second;
    start_service(next_connection);
  }
}

void connection_manager::stop_all() {
  std::lock_guard<std::mutex> lock(data_mutex_);

  for (const std::unique_ptr<connection_handler>& current_service : services_) {
    current_service->stop();
  }

  pending_connection_ids_ = decltype(pending_connection_ids_)();
}

}  // namespace proxy::connection
