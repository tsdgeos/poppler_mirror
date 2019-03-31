//========================================================================
//
// GooCheckedOps.h
//
// This file is licensed under the GPLv2 or later
//
// Copyright (C) 2018 Adam Reichold <adam.reichold@t-online.de>
// Copyright (C) 2019 LE GARREC Vincent <legarrec.vincent@gmail.com>
//
//========================================================================

#ifndef GOO_CHECKED_OPS_H
#define GOO_CHECKED_OPS_H

#include <climits>
#include <limits>

inline bool checkedAssign(long long lz, int *z) {
  static_assert(LLONG_MAX > INT_MAX, "Need type larger than int to perform overflow checks.");

  if (lz > INT_MAX || lz < INT_MIN) {
    return true;
  }

  *z = static_cast<int>(lz);
  return false;
}

#ifndef __has_builtin
  #define __has_builtin(x) 0
#endif

inline bool checkedAdd(int x, int y, int *z) {
#if __GNUC__ >= 5 || __has_builtin(__builtin_sadd_overflow)
  return __builtin_sadd_overflow(x, y, z);
#else
  const auto lz = static_cast<long long>(x) + static_cast<long long>(y);
  return checkedAssign(lz, z);
#endif
}

inline bool checkedMultiply(int x, int y, int *z) {
#if __GNUC__ >= 5 || __has_builtin(__builtin_smul_overflow)
  return __builtin_smul_overflow(x, y, z);
#else
  const auto lz = static_cast<long long>(x) * static_cast<long long>(y);
  return checkedAssign(lz, z);
#endif
}

template<typename T> inline T safeAverage(T a, T b) {
  static_assert(std::numeric_limits<long long>::max() > std::numeric_limits<T>::max(),
    "The max of long long type must be larger to perform overflow checks.");
  static_assert(std::numeric_limits<long long>::min() < std::numeric_limits<T>::min(),
    "The min of long long type must be smaller to perform overflow checks.");

  return static_cast<T>((static_cast<long long>(a) + static_cast<long long>(b)) / 2);
}

#endif // GOO_CHECKED_OPS_H
