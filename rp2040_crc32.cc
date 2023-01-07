//---------------------------------------------------------------------------

#include "rp2040_crc32.h"
#include "rp2040_spinlock.h"

//---------------------------------------------------------------------------

struct Rp2040DmaSniff {
  volatile uint32_t control;
  volatile uint32_t data;
};

static Rp2040DmaSniff *const sniff = (Rp2040DmaSniff *)0x50000434;

//---------------------------------------------------------------------------

void Rp2040Crc32::Initialize() {
  // Writing to ROM address 0 seems to work fine as a no-op.
  dma0->destination = 0;

  // Trigger every sys_clk.
  *dmaTimer0 = 0x10001;

  sniff->control = (1 << 11) | // OUT_INV
                   (1 << 10) | // OUT_REV
                   (1 << 5) |  // CALC = CRC-32 with bit reverse data.
                   (0 << 1) |  // DMACH = dma0
                   1;          // EN
}

uint32_t Rp2040Crc32::Crc32(const void *data, size_t length) {
  bool use32BitTransfer = ((intptr_t(data) | length) & 3) == 0;

  spinlock16->Lock();

  sniff->data = 0xffffffff;

  dma0->source = data;
  if (use32BitTransfer) {
    dma0->count = length >> 2;
    dma0->controlTrigger = (1 << 23) |    // SNIFF_EN
                           (0x3b << 15) | // TREQ_SEL = Timer0
                           (0 << 11) |    // CHAIN_TO = dma0 = Disabled
                           (0 << 5) |     // INCR_WRITE
                           (1 << 4) |     // INCR_READ
                           (2 << 2) |     // DATA_SIZE = SIZE_WORD
                           1;             // EN
  } else {
    dma0->count = length;
    dma0->controlTrigger = (1 << 23) |    // SNIFF_EN
                           (0x3b << 15) | // TREQ_SEL = Timer0
                           (0 << 11) |    // CHAIN_TO = dma0 = Disabled
                           (0 << 5) |     // INCR_WRITE
                           (1 << 4) |     // INCR_READ
                           (0 << 2) |     // DATA_SIZE = SIZE_BYTE
                           1;             // EN
  }

  dma0->WaitUntilComplete();
  uint32_t value = sniff->data;

  spinlock16->Unlock();

  return value;
}

//---------------------------------------------------------------------------

uint32_t Crc32(const void *v, size_t count) {
  return Rp2040Crc32::Crc32(v, count);
}

//---------------------------------------------------------------------------
