/*********************************************

  Copyright (c) Jackson Nestelroad 2022
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <compare>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

// Implementation of RFC 4122.

namespace util {

class uuid_exception : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

constexpr std::size_t uuids_per_tick = 1024;
using uuid_time_t = std::uint64_t;
struct uuid_node_t {
  std::uint8_t node_id[6];
};

struct uuid_state_t {
  uuid_time_t timestamp;
  uuid_node_t node;
  std::uint16_t clock_sequence;
};

// A universally-unique identifier, represented as a 128-bit integer.
struct uuid_t {
  std::uint32_t time_low;
  std::uint16_t time_mid;
  std::uint16_t time_hi_and_version;
  std::uint8_t clock_seq_hi_and_reserved;
  std::uint8_t clock_seq_low;
  std::uint8_t node[6];

  constexpr std::uint32_t node_hi() const { return (node[0] << 24) | (node[1] << 16) | (node[2] << 8) | node[3]; }
  constexpr std::uint16_t node_lo() const { return (node[4] << 8) | node[5]; }

  std::string to_string() const;

  std::strong_ordering operator<=>(const uuid_t& rhs) const;

  inline bool operator==(const uuid_t& rhs) const { return (*this <=> rhs) == 0; }
  inline bool operator!=(const uuid_t& rhs) const { return (*this <=> rhs) != 0; }
  inline bool operator<(const uuid_t& rhs) const { return (*this <=> rhs) < 0; }
  inline bool operator>(const uuid_t& rhs) const { return (*this <=> rhs) > 0; }
  inline bool operator<=(const uuid_t& rhs) const { return (*this <=> rhs) <= 0; }
  inline bool operator>=(const uuid_t& rhs) const { return (*this <=> rhs) >= 0; }
};

inline std::ostream& operator<<(std::ostream& out, const uuid_t& uuid) {
  out << uuid.to_string();
  return out;
}

static_assert(sizeof(uuid_t) == 16, "UUID is expected to be 16 bytes");

static constexpr std::size_t md5_hash_size = 16;
using md5_hash = std::array<std::uint8_t, md5_hash_size>;

static constexpr std::size_t sha1_hash_size = 20;
using sha1_hash = std::array<std::uint8_t, sha1_hash_size>;

static constexpr std::size_t raw_uuid_size = sizeof(uuid_t);
using raw_uuid = std::array<std::uint8_t, raw_uuid_size>;

// A factory for generating universally-unique identifiers.
//
// Instances of this class share the same implementation. In other words, UUIDs are guaranteed to be unique across
// instances of this factory class.
//
// Supports v1, v3, and v5 UUIDs.
class uuid_factory {
 public:
  uuid_factory();

  uuid_t create();
  uuid_t create_md5_from_name(uuid_t namespace_id, std::string_view name);
  uuid_t create_random();
  uuid_t create_sha1_from_name(uuid_t namespace_id, std::string_view name);

  constexpr uuid_t v1() { return create(); }
  constexpr uuid_t v3(uuid_t namespace_id, std::string_view name) { return create_md5_from_name(namespace_id, name); }
  constexpr uuid_t v4() { return create_random(); }
  constexpr uuid_t v5(uuid_t namespace_id, std::string_view name) { return create_md5_from_name(namespace_id, name); }

 private:
  static constexpr std::string_view nodeid_file_name = "nodeid";
  static constexpr std::string_view state_file_name = "state";

  static uuid_node_t get_ieee_node_identifier();
  static std::pair<bool, uuid_state_t> read_state();
  static void write_state(uuid_state_t state);
  static uuid_time_t get_current_time();
  static void initialize_random_number_generation();
  static std::uint16_t random_number();

  static std::filesystem::path uuid_state_directory_;
  static std::mutex mutex_;
  static uuid_node_t saved_node_;
  static uuid_state_t state_;
  static uuid_time_t next_save_;

  static constexpr unsigned int random_number_generation_strength = 128;
  static std::once_flag random_initialization_once_flag_;
  static openssl::ptrs::seed_src_rand_context seed_;
  static openssl::ptrs::ctr_drbg_rand_context rand_context_;

  uuid_t format_uuid_v1(uuid_state_t& state);
  uuid_t format_uuid_v3or5(raw_uuid hash, std::uint8_t version);
};

namespace uuid_ns {

static constexpr uuid_t dns = {0x6ba7b810, 0x9dad, 0x11d1, 0x80, 0xb4, {0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8}};
static constexpr uuid_t url = {0x6ba7b811, 0x9dad, 0x11d1, 0x80, 0xb4, {0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8}};
static constexpr uuid_t oid = {0x6ba7b812, 0x9dad, 0x11d1, 0x80, 0xb4, {0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8}};
static constexpr uuid_t x500 = {0x6ba7b814, 0x9dad, 0x11d1, 0x80, 0xb4, {0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8}};

}  // namespace uuid_ns

}  // namespace util

template <>
struct std::hash<util::uuid_t> {
  std::size_t operator()(const util::uuid_t& uuid) const noexcept;
};
