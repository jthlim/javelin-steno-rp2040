//---------------------------------------------------------------------------

#pragma once
#include <stdint.h>

//---------------------------------------------------------------------------

struct PicoDividerResult {
  int32_t quotient;
  int32_t remainder;
};

#if JAVELIN_PICO_PLATFORM == 2040

struct PicoDivider {
public:
  PicoDivider() = delete;

  [[gnu::always_inline]] void DividerDelay() {
    asm volatile("push { r0, r1, r2, r3 }" : :);
    asm volatile("pop { r0, r1, r2, r3 }" : :);
  }

  [[gnu::always_inline]] volatile PicoDividerResult &
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
  volatile PicoDividerResult result;
};

static PicoDivider *const divider = (PicoDivider *)0xd0000060;

#else

struct PicoDivider {
public:
  [[gnu::always_inline]] static PicoDividerResult Divide(uint32_t numerator,
                                                         uint32_t denominator) {
    return (PicoDividerResult){
        .quotient = int32_t(numerator / denominator),
        .remainder = int32_t(numerator % denominator),
    };
  }
};

static PicoDivider *const divider = (PicoDivider *)0;

#endif

//---------------------------------------------------------------------------
