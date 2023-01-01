//---------------------------------------------------------------------------

#pragma once
#include <stdint.h>

//---------------------------------------------------------------------------

struct Rp2040DividerResult {
  int32_t quotient;
  int32_t remainder;
};

struct Rp2040Divider {
public:
  Rp2040Divider() = delete;

  __attribute__((always_inline)) void DividerDelay() {
    asm volatile("push { r0, r1, r2 }" : :);
    asm volatile("pop { r0, r1, r2 }" : :);
  }

  __attribute__((always_inline)) volatile Rp2040DividerResult &
  Divide(uint32_t numerator, uint32_t denominator) {
    udividend = numerator;
    udivisor = denominator;
    DividerDelay();
    return result;
  }

private:
  volatile uint32_t udividend;
  volatile uint32_t udivisor;
  volatile int32_t sdividend;
  volatile int32_t sdivisor;
  volatile Rp2040DividerResult result;
};

static Rp2040Divider *const divider = (Rp2040Divider *)0xd0000060;

//---------------------------------------------------------------------------
