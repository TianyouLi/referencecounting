#pragma once

#include <atomic>
#include <typeinfo>
#include <optional>
#include <type_traits>

#include <iostream>

template <typename T>
  requires std::signed_integral<T>
class ReferenceCount {
  public:
    enum Status { DEAD, ALIVE, NOREF, OVERFLOW, UNEXPECTED };
    using InvalidReferenceCountType = int8_t;
  public:
    ReferenceCount() : _refcnt(REF_ONE) {
      static_assert(!std::is_same_v<REFCNT_T, ReferenceCount::InvalidReferenceCountType>);
      std::cout << std::hex << std::uppercase << std::showbase << std::setw(sizeof(REFCNT_T)*2) << std::setfill('0');
      std::cout << "REF_ONE: " << REF_ONE << std::endl;
      std::cout << "REF_ZERO: " << REF_ZERO << std::endl;
      std::cout << "REF_MAX: " << REF_MAX << std::endl;
      std::cout << "REF_RELEASED: " << REF_RELEASED << std::endl;
      std::cout << "REF_SATURATED: " << REF_SATURATED << std::endl;
      std::cout << "REF_DEAD: " << REF_DEAD << std::endl;
      std::cout << std::dec;
    }

  Status get() {
    REFCNT_T cnt = _refcnt.fetch_add(1, std::memory_order_relaxed) +1;
    if (cnt >= (REFCNT_T)REF_ONE) {
      return Status::ALIVE;
    }

    if ((REFCNT_T_UNSIGNED)cnt >= REF_RELEASED) {
      _refcnt.store(REF_DEAD);
      return Status::DEAD;
    }
    if ((REFCNT_T_UNSIGNED)cnt >= REF_MAX) {
      _refcnt.store(REF_SATURATED);
      return Status::OVERFLOW;
    }
    return Status::UNEXPECTED;
  }

  Status put() {
    REFCNT_T cnt = _refcnt.fetch_add(-1, std::memory_order_relaxed) -1;
    if ( cnt >= (REFCNT_T)REF_ONE) {
      return Status::ALIVE;
    }

    if ( (REFCNT_T_UNSIGNED)cnt == REF_ZERO) {
      REFCNT_T expected = REF_ZERO;
      if (_refcnt.compare_exchange_weak(expected,REF_DEAD))
        return Status::NOREF;
      cnt = expected;
    }

    if ((REFCNT_T_UNSIGNED)cnt >= REF_RELEASED) {
      _refcnt.store(REF_DEAD);
      return Status::DEAD;
    }
    if ((REFCNT_T_UNSIGNED)cnt >= REF_MAX) {
      _refcnt.store(REF_SATURATED);
      return Status::OVERFLOW;
    }
    if ((REFCNT_T_UNSIGNED)cnt >= REF_ONE)
      return Status::ALIVE; 

    return Status::UNEXPECTED;
  }

  private:
    using REFCNT_T = std::conditional<sizeof(T) == sizeof(int16_t),  int16_t, 
            typename std::conditional<sizeof(T) == sizeof(int32_t), int32_t, 
            typename std::conditional<sizeof(T) == sizeof(int64_t), int64_t, 
            ReferenceCount::InvalidReferenceCountType
            >::type>::type>::type; 
    using REFCNT_T_UNSIGNED = std::make_unsigned_t<REFCNT_T>;
  
    static const REFCNT_T_UNSIGNED REF_ONE        = 0;  
    static const REFCNT_T_UNSIGNED REF_MAX        = static_cast<REFCNT_T_UNSIGNED>(-1) >> 1;
    static const REFCNT_T_UNSIGNED REF_SATURATED  = static_cast<REFCNT_T_UNSIGNED>(0xA0) << ((sizeof(REFCNT_T_UNSIGNED)-1)*8);
    static const REFCNT_T_UNSIGNED REF_RELEASED   = static_cast<REFCNT_T_UNSIGNED>(0xC0) << ((sizeof(REFCNT_T_UNSIGNED)-1)*8);
    static const REFCNT_T_UNSIGNED REF_DEAD       = static_cast<REFCNT_T_UNSIGNED>(0xE0) << ((sizeof(REFCNT_T_UNSIGNED)-1)*8);
    static const REFCNT_T_UNSIGNED REF_ZERO       = static_cast<REFCNT_T_UNSIGNED>(-1);

    std::atomic<REFCNT_T> _refcnt;
};

