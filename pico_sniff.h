//---------------------------------------------------------------------------

#pragma once
#include <stdint.h>
#include <string.h>

//---------------------------------------------------------------------------

struct PicoDmaSniffControl {
  enum class Calculate : uint32_t {
    CRC_32 = 0,
    BIT_REVERSED_CRC_32 = 1,
    CRC_16_CCITT = 2,
    BIT_REVERSED_CRC_16_CCITT = 3,
    CHECKSUM = 0xf,
  };

  uint32_t enable : 1;
  uint32_t dmaChannel : 4;
  Calculate calculate : 4;
  uint32_t bswap : 1;
  uint32_t bitReverseOutput : 1;
  uint32_t bitInvertOutput : 1;
  uint32_t _reserved12 : 20;

  void operator=(const PicoDmaSniffControl &control) volatile {
    *(volatile uint32_t *)this = *(uint32_t *)&control;
  }
};
static_assert(sizeof(PicoDmaSniffControl) == 4,
              "Unexpected DmaSniffControl size");

struct PicoDmaSniff {
  volatile PicoDmaSniffControl control;
  volatile uint32_t data;
};

#if JAVELIN_PICO_PLATFORM == 2350
static PicoDmaSniff *const sniff = (PicoDmaSniff *)0x50000454;
#elif JAVELIN_PICO_PLATFORM == 2040
static PicoDmaSniff *const sniff = (PicoDmaSniff *)0x50000434;
#else
#error Unsupported platform
#endif

//---------------------------------------------------------------------------
