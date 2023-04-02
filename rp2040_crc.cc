//---------------------------------------------------------------------------

#include "rp2040_crc.h"
#include "rp2040_dma.h"
#include "rp2040_sniff.h"
#include "rp2040_spinlock.h"

//---------------------------------------------------------------------------

void Rp2040Crc::Initialize() {
  // Writing to ROM address 0 seems to work fine as a no-op.
  dma0->destination = 0;
}

uint32_t Rp2040Crc::Crc32(const void *data, size_t length) {
#if JAVELIN_THREADS
  spinlock16->Lock();
#endif

  dma0->source = data;

  sniff->data = 0xffffffff;
  Rp2040DmaSniffControl sniffControl = {
      .enable = true,
      .dmaChannel = 0,
      .calculate = Rp2040DmaSniffControl::Calculate::BIT_REVERSED_CRC_32,
      .bitReverseOutput = true,
      .bitInvertOutput = true,
  };
  sniff->control = sniffControl;

  bool use32BitTransfer = ((intptr_t(data) | length) & 3) == 0;
  Rp2040DmaControl control;
  if (use32BitTransfer) {
    length >>= 2;
    constexpr Rp2040DmaControl dmaControl32BitTransfer = {
        .enable = true,
        .dataSize = Rp2040DmaControl::DataSize::WORD,
        .incrementRead = true,
        .incrementWrite = false,
        .chainToDma = 0,
        .transferRequest = Rp2040DmaTransferRequest::PERMANENT,
        .sniffEnable = true,
    };
    control = dmaControl32BitTransfer;
  } else {
    constexpr Rp2040DmaControl dmaControl8BitTransfer = {
        .enable = true,
        .dataSize = Rp2040DmaControl::DataSize::BYTE,
        .incrementRead = true,
        .incrementWrite = false,
        .chainToDma = 0,
        .transferRequest = Rp2040DmaTransferRequest::PERMANENT,
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

uint32_t Rp2040Crc::Crc16Ccitt(const void *data, size_t length) {
#if JAVELIN_THREADS
  spinlock16->Lock();
#endif

  dma0->source = data;
  dma0->count = length;

  sniff->data = 0xffff;

  Rp2040DmaSniffControl sniffControl = {
      .enable = true,
      .dmaChannel = 0,
      .calculate = Rp2040DmaSniffControl::Calculate::CRC_16_CCITT,
      .bitReverseOutput = false,
      .bitInvertOutput = false,
  };
  sniff->control = sniffControl;

  constexpr Rp2040DmaControl controlTrigger = {
      .enable = true,
      .dataSize = Rp2040DmaControl::DataSize::BYTE,
      .incrementRead = true,
      .incrementWrite = false,
      .chainToDma = 0,
      .transferRequest = Rp2040DmaTransferRequest::PERMANENT,
      .sniffEnable = true,
  };
  dma0->controlTrigger = controlTrigger;

  dma0->WaitUntilComplete();
  uint32_t value = sniff->data;

#if JAVELIN_THREADS
  spinlock16->Unlock();
#endif

  return value;
}

//---------------------------------------------------------------------------

uint32_t Crc32(const void *v, size_t count) {
  return Rp2040Crc::Crc32(v, count);
}

uint32_t Crc16Ccitt(const void *v, size_t count) {
  return Rp2040Crc::Crc16Ccitt(v, count);
}

//---------------------------------------------------------------------------
