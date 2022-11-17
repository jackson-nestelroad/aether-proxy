/*********************************************

  Copyright (c) Jackson Nestelroad 2020
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <exception>

#include "aether/util/console.hpp"

#ifndef NDEBUG
#define AETHER_ASSERT(condition, message)                                                                       \
  do {                                                                                                          \
    if (!(condition)) {                                                                                         \
      ::out::safe_error::stream("Assertion `" #condition "` failed in ", __FILE__, " at line ", __LINE__, ": ", \
                                message, ::out::manip::endl);                                                   \
      ::std::terminate();                                                                                       \
    }                                                                                                           \
  } while (false);
#else
#define AETHER_ASSERT(condition, message)                                          \
  do {                                                                             \
    if (!(condition)) {                                                            \
      ::out::safe_error::stream("Program crashed: ", message, ::out::manip::endl); \
      ::std::terminate();                                                          \
    }                                                                              \
  } while (false);
#endif
