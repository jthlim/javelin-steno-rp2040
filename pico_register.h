//---------------------------------------------------------------------------

#pragma once
#include <stdint.h>

//---------------------------------------------------------------------------

struct PicoRegister {
  volatile uint32_t value;

  void operator=(uint32_t newValue) { value = newValue; }
  operator uint32_t() const { return value; }

  void operator^=(uint32_t xorMask) { SetAlias(0x1000, xorMask); }
  void operator|=(uint32_t setMask) { SetAlias(0x2000, setMask); }
  void operator&=(uint32_t mask) { SetAlias(0x3000, ~mask); }

  void SetAlias(uint32_t aliasOffset, uint32_t mask) {
    volatile uint32_t *alias = (uint32_t *)(aliasOffset + uintptr_t(&value));
    *alias = mask;
  }
};

//---------------------------------------------------------------------------
