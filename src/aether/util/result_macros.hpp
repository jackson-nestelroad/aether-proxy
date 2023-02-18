/*********************************************

  Copyright (c) Jackson Nestelroad 2023
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <utility>

#include "aether/util/console.hpp"
#include "aether/util/result.hpp"

#define RETURN_IF_ERROR(rexpr)                       \
  {                                                  \
    auto __result = (rexpr);                         \
    if (__result.is_err()) {                         \
      return {util::err, std::move(__result).err()}; \
    }                                                \
  }

#define __RESULT_MACROS_CONCAT_NAME_INNER(x, y) x##y
#define __RESULT_MACROS_CONCAT_NAME(x, y) __RESULT_MACROS_CONCAT_NAME_INNER(x, y)

#define __ASSIGN_OR_RETURN_IMPL(temp, lhs, rexpr) \
  auto temp = (rexpr);                            \
  if (temp.is_err()) {                            \
    return {util::err, std::move(temp).err()};    \
  }                                               \
  lhs = std::move(temp).ok();

#define ASSIGN_OR_RETURN(lhs, rexpr) \
  __ASSIGN_OR_RETURN_IMPL(__RESULT_MACROS_CONCAT_NAME(__result, __COUNTER__), lhs, rexpr);

#define DIE_IF_ERROR(rexpr)      \
  {                              \
    auto __result = (rexpr);     \
    if (__result.is_err()) {     \
      out::fatal::log(__result); \
    }                            \
  }
