/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#include "console.hpp"

namespace out {
std::ostream terminal(std::cout.rdbuf());

logging_manager::logging_manager()
    : silenced_(false), original_cout_buf_(std::cout.rdbuf()), original_cerr_buf_(std::cerr.rdbuf()) {}

logging_manager::~logging_manager() { reset(); }

void logging_manager::reset_buffers() {
  if (file_stream_) {
    std::cout.rdbuf(file_stream_->rdbuf());
    std::cerr.rdbuf(file_stream_->rdbuf());
  } else {
    std::cout.rdbuf(original_cout_buf_);
    std::cerr.rdbuf(original_cerr_buf_);
  }
}

void logging_manager::reset_flags() {
  std::cout.clear();
  std::cerr.clear();
}

void logging_manager::redirect_to_file(const std::string& file_name) {
  auto new_file_stream = std::make_unique<std::ofstream>(file_name);
  if (new_file_stream->good()) {
    if (!silenced_) {
      std::cout.rdbuf(new_file_stream->rdbuf());
      std::cerr.rdbuf(new_file_stream->rdbuf());
      reset_flags();
    }

    file_stream_.swap(new_file_stream);
  }
}

void logging_manager::unsync_with_stdio() {
  std::cout.sync_with_stdio(false);
  std::cerr.sync_with_stdio(false);
}

void logging_manager::silence() {
  silenced_ = true;
  std::cout.rdbuf(cnull_.rdbuf());
  std::cerr.rdbuf(cnull_.rdbuf());
}

void logging_manager::unsilence() {
  silenced_ = false;
  reset_buffers();
  reset_flags();
}

void logging_manager::reset() {
  file_stream_.reset();
  std::cout.sync_with_stdio(true);
  std::cerr.sync_with_stdio(true);
  reset_buffers();
  reset_flags();
}

const manip_t manip::endl = static_cast<manip_t>(std::endl);
const manip_t manip::ends = static_cast<manip_t>(std::ends);
const manip_t manip::flush = static_cast<manip_t>(std::flush);

}  // namespace out
