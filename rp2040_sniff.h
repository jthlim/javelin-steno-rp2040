//---------------------------------------------------------------------------

#pragma once
#include <stdint.h>
#include <string.h>

//---------------------------------------------------------------------------

struct Rp2040DmaSniffControl {
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

  void operator=(const Rp2040DmaSniffControl &control) volatile {
    memcpy((void *)this, &control, sizeof(Rp2040DmaSniffControl));
  }
};
static_assert(sizeof(Rp2040DmaSniffControl) == 4,
              "Unexpected DmaSniffControl size");

struct Rp2040DmaSniff {
  volatile Rp2040DmaSniffControl control;
  volatile uint32_t data;
};

static Rp2040DmaSniff *const sniff = (Rp2040DmaSniff *)0x50000434;

//---------------------------------------------------------------------------
