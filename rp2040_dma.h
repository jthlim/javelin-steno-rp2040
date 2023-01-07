//---------------------------------------------------------------------------

#pragma once
#include <stdint.h>

//---------------------------------------------------------------------------

struct Rp2040Dma {
  const void *volatile source;
  void *volatile destination;
  volatile uint32_t count;
  volatile uint32_t controlTrigger;

  // Alias 1
  volatile uint32_t controlAlias1;
  const void *volatile sourceAlias1;
  void *volatile destinationAlias1;
  volatile uint32_t countTrigger;

  // Alias 2
  volatile uint32_t controlAlias2;
  volatile uint32_t countAlias2;
  const void *volatile sourceAlias2;
  void *volatile destinationTrigger;

  // Alias 3
  volatile uint32_t controlAlias3;
  void *volatile destinationAlias3;
  volatile uint32_t countAlias3;
  const void *volatile sourceTrigger;

  inline bool IsBusy() const { return (controlAlias1 & (1 << 24)) != 0; }

  inline void WaitUntilComplete() const {
    while (IsBusy()) {
    }
  }
};

static Rp2040Dma *const dma0 = (Rp2040Dma *)0x50000000;
static Rp2040Dma *const dma1 = (Rp2040Dma *)0x50000040;
static Rp2040Dma *const dma2 = (Rp2040Dma *)0x50000080;
static Rp2040Dma *const dma3 = (Rp2040Dma *)0x500000c0;
static Rp2040Dma *const dma4 = (Rp2040Dma *)0x50000100;
static Rp2040Dma *const dma5 = (Rp2040Dma *)0x50000140;
static Rp2040Dma *const dma6 = (Rp2040Dma *)0x50000180;
static Rp2040Dma *const dma7 = (Rp2040Dma *)0x500001c0;
static Rp2040Dma *const dma8 = (Rp2040Dma *)0x50000200;
static Rp2040Dma *const dma9 = (Rp2040Dma *)0x50000240;
static Rp2040Dma *const dma10 = (Rp2040Dma *)0x50000280;
static Rp2040Dma *const dma11 = (Rp2040Dma *)0x500002c0;

// Upper 16 bits = numerator.
// Lower 16 bits = denominator.
static volatile uint32_t *const dmaTimer0 = (uint32_t *)0x50000420;
static volatile uint32_t *const dmaTimer1 = (uint32_t *)0x50000424;
static volatile uint32_t *const dmaTimer2 = (uint32_t *)0x50000428;
static volatile uint32_t *const dmaTimer3 = (uint32_t *)0x5000042c;

//---------------------------------------------------------------------------
