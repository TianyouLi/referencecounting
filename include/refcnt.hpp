#pragma once

#include <atomic>
#include <typeinfo>
#include <optional>
#include <type_traits>

#include <stdio.h>

template <typename T>
  requires std::signed_integral<T>
class ReferenceCount {
 public:
  enum Status { DEAD, ALIVE, NOREF, OVERFLOW };
  using InvalidReferenceCountType = int8_t;
 public:
  ReferenceCount() : _refcnt(REF_ONE) {
    static_assert(!std::is_same_v<REFCNT_T, ReferenceCount::InvalidReferenceCountType>);
    printf("ONE: %016lX, ZERO: %016lX, MAX: %016lX, RELEASED: %016lX, REF_SATURATED: %016lX, REF_DEAD: %016lX\n", 
      REF_ONE, REF_ZERO, REF_MAX, REF_RELEASED, REF_SATURATED, REF_DEAD);
  }

  Status get() {
    _refcnt.fetch_add(1);
    return Status::ALIVE;
  }

  Status put() {
    _refcnt.fetch_add(-1);
    return Status::DEAD;
  }

 private:
  using REFCNT_T = std::conditional<sizeof(T) == sizeof(int16_t),  int16_t, 
          typename std::conditional<sizeof(T) == sizeof(int32_t), int32_t, 
          typename std::conditional<sizeof(T) == sizeof(int64_t), int64_t, 
          ReferenceCount::InvalidReferenceCountType
          >::type>::type>::type; 
  using REFCNT_T_UNSIGNED = std::make_unsigned_t<REFCNT_T>;
  
  static const REFCNT_T_UNSIGNED REF_ONE = 0;  
  static const REFCNT_T_UNSIGNED REF_ZERO = static_cast<REFCNT_T_UNSIGNED>(-1);   
  static const REFCNT_T_UNSIGNED REF_MAX = REF_ZERO >> 1;
  static const REFCNT_T_UNSIGNED REF_RELEASED = REF_MAX + ((REF_ZERO - REF_MAX) >> 1);
  static const REFCNT_T_UNSIGNED REF_SATURATED = REF_MAX + ((REF_RELEASED - REF_MAX) >> 1);
  static const REFCNT_T_UNSIGNED REF_DEAD = REF_RELEASED + ((REF_ZERO - REF_RELEASED) >> 1);

  std::atomic<REFCNT_T> _refcnt;
};

