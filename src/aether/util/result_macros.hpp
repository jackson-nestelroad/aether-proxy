/*********************************************

  Copyright (c) Jackson Nestelroad 2023
  jackson.nestelroad.com

*********************************************/

#pragma once

#include <type_traits>
#include <utility>

#include "aether/util/console.hpp"
#include "aether/util/result.hpp"

#define RETURN_IF_ERROR(rexpr)                      \
  if (auto __result = (rexpr); __result.is_err()) { \
    return {util::err, std::move(__result).err()};  \
  }

#define ASSIGN_OR_RETURN(lhs, rexpr) \
  __ASSIGN_OR_RETURN_IMPL(__RESULT_MACROS_CONCAT_NAME(__result, __COUNTER__), lhs, rexpr);

#define __RESULT_MACROS_CONCAT_NAME_INNER(x, y) x##y
#define __RESULT_MACROS_CONCAT_NAME(x, y) __RESULT_MACROS_CONCAT_NAME_INNER(x, y)

#define __ASSIGN_OR_RETURN_IMPL(result, lhs, rexpr) \
  auto result = (rexpr);                            \
  if (result.is_err()) {                            \
    return {util::err, std::move(result).err()};    \
  }                                                 \
  __RESULT_MACROS_UNPARENTHESIS(lhs) = std::move(result).ok()

#define __RESULT_MACROS_UNPARENTHESIS_INNER(...) __RESULT_MACROS_UNPARENTHESIS_INNER_IMPL(__VA_ARGS__)
#define __RESULT_MACROS_UNPARENTHESIS_INNER_IMPL(...) __RESULT_MACROS_VAN##__VA_ARGS__
#define ISH(...) ISH __VA_ARGS__
#define __RESULT_MACROS_VANISH

#define __RESULT_MACROS_UNPARENTHESIS(...) __RESULT_MACROS_UNPARENTHESIS_INNER(ISH __VA_ARGS__)
