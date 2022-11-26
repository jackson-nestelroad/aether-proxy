/*********************************************

  Copyright (c) Jackson Nestelroad 2022
  jackson.nestelroad.com

*********************************************/

#include "uuid.hpp"

#include <openssl/evp.h>
#include <string.h>

#include <algorithm>
#include <array>
#include <boost/asio.hpp>
#include <compare>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>

#include "aether/util/console.hpp"
#include "aether/util/openssl_ptrs.hpp"

namespace util {

std::filesystem::path uuid_factory::uuid_state_directory_(
    (std::filesystem::path(AETHER_HOME) / ".uid").make_preferred());
std::mutex uuid_factory::mutex_;
uuid_node_t uuid_factory::saved_node_;
uuid_state_t uuid_factory::state_;
uuid_time_t uuid_factory::next_save_;
std::once_flag uuid_factory::random_initialization_once_flag_;
openssl::ptrs::ctr_drbg_rand_context uuid_factory::rand_context_(openssl::ptrs::in_place);

uuid_factory::uuid_factory() {
  if (!std::filesystem::exists(uuid_state_directory_)) {
    std::filesystem::create_directory(uuid_state_directory_);
  }
}

namespace {

constexpr std::size_t random_info_hostname_size = 257;
struct random_info_t {
  std::uint64_t now;
  char hostname[random_info_hostname_size];
};

md5_hash get_random_info() {
  const EVP_MD* evp_md5 = EVP_md5();
  openssl::ptrs::evp_md_context md_ctx(openssl::ptrs::in_place);
  if (EVP_DigestInit_ex2(*md_ctx, evp_md5, nullptr) <= 0) {
    throw uuid_exception{"Failed to initialize MD5 digest context"};
  }
  random_info_t random_info;
  random_info.now =
      std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch())
          .count();
  std::string hostname = boost::asio::ip::host_name();
  std::memcpy(&random_info.hostname, hostname.data(), std::min(random_info_hostname_size - 1, hostname.size()));
  if (EVP_DigestUpdate(*md_ctx, &random_info, sizeof(random_info)) <= 0) {
    throw uuid_exception{"Failed to hash random info into MD5 digest context"};
  }

  std::uint8_t hashed[EVP_MAX_MD_SIZE];
  unsigned int hashed_length;
  if (EVP_DigestFinal_ex(*md_ctx, hashed, &hashed_length) <= 0) {
    throw uuid_exception{"Failed to finalize MD5 digest context"};
  }

  if (hashed_length != md5_hash_size) {
    throw uuid_exception{"MD5 hash algorithm did not produce expected 128 bits"};
  }

  md5_hash result;
  std::memcpy(result.data(), hashed, md5_hash_size);
  return result;
}

uuid_time_t get_system_time() {
  // UUID UTC base time is October 15, 1582.
  // Unix base time is January 1, 1970.
  auto timestamp = std::chrono::system_clock::now() + std::chrono::nanoseconds(0x01B21DD213814000);
  return std::chrono::duration_cast<std::chrono::nanoseconds>(timestamp.time_since_epoch()).count() / 100;
}

struct uuid_state {
  uuid_time_t timestamp;
  uuid_node_t node;
  std::uint16_t clock_sequence;
};

}  // namespace

uuid_node_t uuid_factory::get_ieee_node_identifier() {
  static std::once_flag once_flag;
  std::call_once(once_flag, [] {
    std::fstream file(uuid_state_directory_ / nodeid_file_name, std::ios::in | std::ios::binary);
    if (!file.good() || !file.read(reinterpret_cast<char*>(&saved_node_), sizeof(saved_node_))) {
      file = std::fstream(uuid_state_directory_ / nodeid_file_name, std::ios::out | std::ios::binary);
      if (!file.good()) {
        throw uuid_exception{"Failed to open node identifier file for writing"};
      }
      md5_hash seed = get_random_info();
      seed[0] |= 0x80;
      std::memcpy(&saved_node_, seed.data(), sizeof(saved_node_));
      if (!file.write(reinterpret_cast<const char*>(&saved_node_), sizeof(saved_node_))) {
        throw uuid_exception{"Failed to write to node identifier file"};
      }
    }
  });

  return saved_node_;
}

std::pair<bool, uuid_state_t> uuid_factory::read_state() {
  static std::once_flag once_flag;
  bool valid = true;
  std::call_once(once_flag, [&valid] {
    std::ifstream file(uuid_state_directory_ / state_file_name, std::ios::binary);
    if (!file.good()) {
      valid = false;
      return;
    }
    if (!file.read(reinterpret_cast<char*>(&state_), sizeof(state_))) {
      throw uuid_exception{"Faield to read UUID factory state from file"};
    }
  });
  return {valid, state_};
}

void uuid_factory::write_state(uuid_state_t state) {
  static std::once_flag once_flag;
  std::call_once(once_flag, [&state] { next_save_ = state.timestamp; });

  state_ = std::move(state);
  if (state_.timestamp >= next_save_) {
    std::ofstream file(uuid_state_directory_ / state_file_name, std::ios::binary);
    if (!file.good() || !file.write(reinterpret_cast<const char*>(&state_), sizeof(state_))) {
      out::safe_warn::log("Failed to write UUID factory state to disk");
    }
    next_save_ = state_.timestamp + (10 * 10 * 1000 * 1000);
  }
}

uuid_time_t uuid_factory::get_current_time() {
  static std::once_flag once_flag;
  static uuid_time_t time_last;
  static std::uint16_t uuids_this_tick;

  std::call_once(once_flag, [] {
    time_last = get_system_time();
    uuids_this_tick = uuids_per_tick;
  });

  uuid_time_t time_now;

  while (true) {
    time_now = get_system_time();
    if (time_last != time_now) {
      // Clock reading changed since last UUID generated.
      // Rset count of UUIDs with this clock reading to 0.
      uuids_this_tick = 0;
      time_last = time_now;
      break;
    }
    if (uuids_this_tick < uuids_per_tick) {
      ++uuids_this_tick;
      break;
    }
    // Else, we are moving too fast for our clock, so we repeat.
  }
  return time_now + uuids_this_tick;
}

void uuid_factory::initialize_random_number_generation() {
  OSSL_PARAM params[2];
  auto* it = params;
  *it++ = OSSL_PARAM_construct_utf8_string("cipher", const_cast<char*>(SN_aes_256_ctr), 0);
  *it = OSSL_PARAM_construct_end();
  if (EVP_RAND_instantiate(*rand_context_, random_number_generation_strength, 0, nullptr, 0, params) <= 0) {
    ERR_print_errors_fp(stdout);
    throw uuid_exception{"Failed to instantiate random number algorithm context"};
  }
}

std::uint16_t uuid_factory::random_number() {
  std::call_once(random_initialization_once_flag_, &uuid_factory::initialize_random_number_generation);
  std::uint16_t result;
  if (EVP_RAND_generate(*rand_context_, reinterpret_cast<std::uint8_t*>(&result), sizeof(result),
                        random_number_generation_strength, 0, nullptr, 0) <= 0) {
    throw uuid_exception{"Failed to generate random number"};
  }
  return result;
}

uuid_t uuid_factory::create() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    // These two variables represent our current state.
    uuid_time_t timestamp = get_current_time();
    uuid_node_t node = get_ieee_node_identifier();

    // This variable represents our previous state.
    auto [valid_state, state] = read_state();

    if (!valid_state || std::memcmp(&node, &state.node, sizeof(node)) != 0) {
      // State is invalid or node identifier changed, so we change the clock sequence.
      state.clock_sequence = random_number();
    } else if (timestamp < state.timestamp) {
      ++state.clock_sequence;
    }
    // Update our state.
    uuid_state_t new_state{
        .timestamp = timestamp,
        .node = node,
        .clock_sequence = state.clock_sequence,
    };
    write_state(std::move(new_state));
  }

  return format_uuid_v1(state_);
}

uuid_t uuid_factory::format_uuid_v1(uuid_state_t& state) {
  uuid_t uuid;
  uuid.time_low = state.timestamp & 0xFFFFFFFF;
  uuid.time_mid = ((state.timestamp >> 32) & 0xFFFF);
  uuid.time_hi_and_version = (state.timestamp >> 48) & 0x0FFF;
  uuid.time_hi_and_version |= (1 << 12);
  uuid.clock_seq_low = state.clock_sequence & 0xFF;
  uuid.clock_seq_hi_and_reserved = (state.clock_sequence & 0x3F00) >> 8;
  uuid.clock_seq_hi_and_reserved |= 0x80;
  std::memcpy(&uuid.node, &state.node, sizeof(uuid.node));
  return uuid;
}

uuid_t uuid_factory::create_md5_from_name(uuid_t namespace_id, std::string_view name) {
  // Put namespace id in network byte order so it hashes the same no matter what endian machine we are on.
  uuid_t net_nsid = namespace_id;
  net_nsid.time_low = htonl(net_nsid.time_low);
  net_nsid.time_mid = htons(net_nsid.time_mid);
  net_nsid.time_hi_and_version = htons(net_nsid.time_hi_and_version);

  const EVP_MD* evp_md5 = EVP_md5();
  openssl::ptrs::evp_md_context md_ctx(openssl::ptrs::in_place);
  if (EVP_DigestInit_ex2(*md_ctx, evp_md5, nullptr) <= 0) {
    throw uuid_exception{"Failed to initialize MD5 digest context"};
  }

  if (EVP_DigestUpdate(*md_ctx, &net_nsid, sizeof(net_nsid)) <= 0) {
    throw uuid_exception{"Failed to hash namespace id into MD5 digest context"};
  }
  if (EVP_DigestUpdate(*md_ctx, name.data(), name.length()) <= 0) {
    throw uuid_exception{"Failed to hash name into MD5 digest context"};
  }

  std::uint8_t hashed[EVP_MAX_MD_SIZE];
  unsigned int hashed_length;
  if (EVP_DigestFinal_ex(*md_ctx, hashed, &hashed_length) <= 0) {
    throw uuid_exception{"Failed to finalize MD5 digest context"};
  }

  if (hashed_length != md5_hash_size) {
    throw uuid_exception{"MD5 hash algorithm did not produce expected 128 bits"};
  }

  md5_hash result;
  std::memcpy(result.data(), hashed, md5_hash_size);
  return format_uuid_v3or5(std::move(result), 3);
}

uuid_t uuid_factory::create_sha1_from_name(uuid_t namespace_id, std::string_view name) {
  // Put namespace id in network byte order so it hashes the same no matter what endian machine we are on.
  uuid_t net_nsid = namespace_id;
  net_nsid.time_low = htonl(net_nsid.time_low);
  net_nsid.time_mid = htons(net_nsid.time_mid);
  net_nsid.time_hi_and_version = htons(net_nsid.time_hi_and_version);

  const EVP_MD* evp_sha1 = EVP_sha1();
  openssl::ptrs::evp_md_context md_ctx(openssl::ptrs::in_place);
  if (EVP_DigestInit_ex2(*md_ctx, evp_sha1, nullptr) <= 0) {
    throw uuid_exception{"Failed to initialize SHA1 digest context"};
  }

  if (EVP_DigestUpdate(*md_ctx, &net_nsid, sizeof(net_nsid)) <= 0) {
    throw uuid_exception{"Failed to hash namespace id into SHA1 digest context"};
  }
  if (EVP_DigestUpdate(*md_ctx, name.data(), name.length()) <= 0) {
    throw uuid_exception{"Failed to hash name into SHA1 digest context"};
  }

  std::uint8_t hashed[EVP_MAX_MD_SIZE];
  unsigned int hashed_length;
  if (EVP_DigestFinal_ex(*md_ctx, hashed, &hashed_length) <= 0) {
    throw uuid_exception{"Failed to finalize SHA1 digest context"};
  }

  if (hashed_length != sha1_hash_size) {
    throw uuid_exception{"SHA1 hash algorithm did not produce expected 160 bits"};
  }

  // We only use 16 bytes of the 20-byte hash.
  raw_uuid result;
  std::memcpy(result.data(), hashed, md5_hash_size);
  return format_uuid_v3or5(std::move(result), 5);
}

uuid_t uuid_factory::create_random() {
  std::call_once(random_initialization_once_flag_, &uuid_factory::initialize_random_number_generation);
  std::uint64_t result[2]{};
  std::uint8_t* bytes = reinterpret_cast<std::uint8_t*>(result);
  if (EVP_RAND_generate(*rand_context_, bytes, sizeof(result), random_number_generation_strength, 0, nullptr, 0) <= 0) {
    throw uuid_exception{"Failed to generate random number"};
  }
  uuid_t uuid = {*reinterpret_cast<std::uint32_t*>(bytes), *reinterpret_cast<std::uint16_t*>(bytes + 4),
                 *reinterpret_cast<std::uint16_t*>(bytes + 6), *reinterpret_cast<std::uint8_t*>(bytes + 8),
                 *reinterpret_cast<std::uint8_t*>(bytes + 9)};
  for (std::size_t i = 0; i < 6; ++i) {
    uuid.node[i] = *(bytes + 10 + i);
  }
  return uuid;
}

uuid_t uuid_factory::format_uuid_v3or5(raw_uuid hash, std::uint8_t version) {
  uuid_t uuid;
  // Convert UUID to local byte order.
  std::memcpy(&uuid, hash.data(), hash.size());
  uuid.time_low = ntohl(uuid.time_low);
  uuid.time_mid = ntohs(uuid.time_mid);
  uuid.time_hi_and_version = ntohs(uuid.time_hi_and_version);

  // Put in variant and version bits.
  uuid.time_hi_and_version &= 0x0FFF;
  uuid.time_hi_and_version |= version << 12;
  uuid.clock_seq_hi_and_reserved &= 0x3F;
  uuid.clock_seq_hi_and_reserved |= 0x80;

  return uuid;
}

std::strong_ordering uuid_t::operator<=>(const uuid_t& rhs) const {
  auto ret =
      std::tie(time_low, time_mid, time_hi_and_version, clock_seq_hi_and_reserved, clock_seq_low) <=>
      std::tie(rhs.time_low, rhs.time_mid, rhs.time_hi_and_version, rhs.clock_seq_hi_and_reserved, rhs.clock_seq_low);
  if (ret != 0) {
    return ret;
  }
  for (std::size_t i = 0; i < 6; ++i) {
    auto ret = node[i] <=> rhs.node[i];
    if (ret != 0) {
      return ret;
    }
  }
  return std::strong_ordering::equal;
}

std::string uuid_t::to_string() const {
  std::stringstream str;
  str << std::hex << std::setfill('0') << std::setw(sizeof(time_low) * 2) << time_low << "-"
      << std::setw(sizeof(time_mid) * 2) << time_mid << "-" << std::setw(sizeof(time_hi_and_version) * 2)
      << time_hi_and_version << "-" << std::setw(sizeof(clock_seq_hi_and_reserved) * 2)
      << static_cast<int>(clock_seq_hi_and_reserved) << "-" << std::setw(sizeof(clock_seq_low) * 2)
      << static_cast<int>(clock_seq_low) << "-";
  for (std::size_t i = 0; i < 6; ++i) {
    str << std::setw(sizeof(node[i]) * 2) << static_cast<int>(node[i]);
  }
  return str.str();
}

}  // namespace util

namespace {

template <typename T, typename... Rest>
void hash_combine(std::size_t& seed, const T& v, const Rest&... rest) {
  seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  (hash_combine(seed, rest), ...);
}

}  // namespace

std::size_t std::hash<util::uuid_t>::operator()(const util::uuid_t& uuid) const noexcept {
  std::size_t hash = 0;
  hash_combine(hash, uuid.time_low, uuid.time_mid, uuid.time_hi_and_version, uuid.clock_seq_hi_and_reserved,
               uuid.clock_seq_low, uuid.node_hi(), uuid.node_lo());
  return hash;
}
