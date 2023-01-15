//---------------------------------------------------------------------------

#include "rp2040_crc32.h"
#include "rp2040_dma.h"
#include "rp2040_spinlock.h"

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

void Rp2040Crc32::Initialize() {
  // Writing to ROM address 0 seems to work fine as a no-op.
  dma0->destination = 0;

  // Trigger every sys_clk.
  *dmaTimer0 = 0x10001;

  Rp2040DmaSniffControl sniffControl = {
      .enable = true,
      .dmaChannel = 0,
      .calculate = Rp2040DmaSniffControl::Calculate::BIT_REVERSED_CRC_32,
      .bitReverseOutput = true,
      .bitInvertOutput = true,
  };
  sniff->control = sniffControl;
}

uint32_t Rp2040Crc32::Crc32(const void *data, size_t length) {
#if JAVELIN_THREADS
  spinlock16->Lock();
#endif

  sniff->data = 0xffffffff;

  dma0->source = data;

  bool use32BitTransfer = ((intptr_t(data) | length) & 3) == 0;
  Rp2040DmaControl control;
  if (use32BitTransfer) {
    length >>= 2;
    Rp2040DmaControl dmaControl32BitTransfer = {
        .enable = true,
        .dataSize = Rp2040DmaControl::DataSize::WORD,
        .incrementRead = true,
        .incrementWrite = false,
        .chainToDma = 0,
        .transferRequest = Rp2040DmaControl::TransferRequest::TIMER_0,
        .sniffEnable = true,
    };
    control = dmaControl32BitTransfer;
  } else {
    Rp2040DmaControl dmaControl8BitTransfer = {
        .enable = true,
        .dataSize = Rp2040DmaControl::DataSize::BYTE,
        .incrementRead = true,
        .incrementWrite = false,
        .chainToDma = 0,
        .transferRequest = Rp2040DmaControl::TransferRequest::TIMER_0,
        .sniffEnable = true,
    };

    control = dmaControl8BitTransfer;
  }
  dma0->count = length;
  dma0->controlTrigger = control;

  dma0->WaitUntilComplete();
  uint32_t value = sniff->data;

#if JAVELIN_THREADS
  spinlock16->Unlock();
#endif

  return value;
}

//---------------------------------------------------------------------------

uint32_t Crc32(const void *v, size_t count) {
  return Rp2040Crc32::Crc32(v, count);
}

//---------------------------------------------------------------------------
