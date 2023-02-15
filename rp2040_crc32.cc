//---------------------------------------------------------------------------

#include "rp2040_crc32.h"
#include "rp2040_dma.h"
#include "rp2040_sniff.h"
#include "rp2040_spinlock.h"

//---------------------------------------------------------------------------

void Rp2040Crc32::Initialize() {
  // Writing to ROM address 0 seems to work fine as a no-op.
  dma0->destination = 0;

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
        .transferRequest = Rp2040DmaControl::TransferRequest::PERMANENT,
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
        .transferRequest = Rp2040DmaControl::TransferRequest::PERMANENT,
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
