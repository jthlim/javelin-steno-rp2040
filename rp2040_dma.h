//---------------------------------------------------------------------------

#pragma once
#include <stdint.h>
#include <string.h>

//---------------------------------------------------------------------------

struct Rp2040DmaControl {
  enum class TransferRequest : uint32_t {
    PIO0_TX0 = 0,
    PIO0_TX1,
    PIO0_TX2,
    PIO0_TX3,
    PIO0_RX0,
    PIO0_RX1,
    PIO0_RX2,
    PIO0_RX3,
    PIO1_TX0,
    PIO1_TX1,
    PIO1_TX2,
    PIO1_TX3,
    PIO1_RX0,
    PIO1_RX1,
    PIO1_RX2,
    PIO1_RX3,
    SPI0_TX,
    SPI0_RX,
    SPI1_TX,
    SPI1_RX,
    UART0_TX,
    UART0_RX,
    UART1_TX,
    UART1_RX,
    PWM_WRAP0,
    PWM_WRAP1,
    PWM_WRAP2,
    PWM_WRAP3,
    PWM_WRAP4,
    PWM_WRAP5,
    PWM_WRAP6,
    PWM_WRAP7,
    I2C0_TX,
    I2C0_RX,
    I2C1_TX,
    I2C1_RX,
    ADC,
    XIP_STREAM,
    XIP_SSITX,
    XIP_SSIRX,
    TIMER_0 = 0x3b,
    TIMER_1 = 0x3c,
    TIMER_2 = 0x3d,
    TIMER_3 = 0x3e,
    PERMANENT = 0x3f,
  };

  enum class DataSize : uint32_t {
    BYTE = 0,
    HALF_WORD = 1,
    WORD = 2,
  };

  uint32_t enable : 1;
  uint32_t highPriority : 1;
  DataSize dataSize : 2;
  uint32_t incrementRead : 1;
  uint32_t incrementWrite : 1;
  uint32_t ringSizeShift : 4;
  uint32_t ringSel : 1;
  uint32_t chainToDma : 4;
  TransferRequest transferRequest : 6;
  uint32_t quietIrq : 1;
  uint32_t bswap : 1;
  uint32_t sniffEnable : 1;
  uint32_t busy : 1;
  uint32_t _reserved25 : 4;
  uint32_t writeError : 1;
  uint32_t readError : 1;
  uint32_t ahbError : 1;

  void operator=(const Rp2040DmaControl &control) volatile {
    memcpy((void *)this, &control, sizeof(Rp2040DmaControl));
  }
};
static_assert(sizeof(Rp2040DmaControl) == 4, "Unexpected DmaControl size");

struct Rp2040DmaIrqControl {
  struct Irq {
    volatile uint32_t enableMask;
    volatile uint32_t forceMask;
    volatile uint32_t status;

    uint32_t AckAllIrqs() {
      uint32_t mask = status;
      status = mask;
      return mask;
    }
    void AckIrq(int dmaChannel) { status = (1 << dmaChannel); }
    void EnableIrq(int dmaChannel) { enableMask |= (1 << dmaChannel); }
  };

  volatile uint32_t status;
  Irq irq0;
  uint32_t _unused10;
  Irq irq1;
};

static Rp2040DmaIrqControl *const dmaIrqControl =
    (Rp2040DmaIrqControl *)0x50000400;

struct Rp2040DmaAbort {
  volatile uint32_t value;

  void Abort(int channelIndex) {
    value = (1 << channelIndex);
    while (value) {
    }
  }
};

static Rp2040DmaAbort *const dmaAbort = (Rp2040DmaAbort *)0x50000444;

struct Rp2040Dma {
  const volatile void *volatile source;
  const volatile void *volatile destination;
  volatile uint32_t count;
  volatile Rp2040DmaControl controlTrigger;

  // Alias 1
  union {
    volatile Rp2040DmaControl control;
    volatile Rp2040DmaControl controlAlias1;
  };
  const volatile void *volatile sourceAlias1;
  const volatile void *volatile destinationAlias1;
  volatile uint32_t countTrigger;

  // Alias 2
  volatile Rp2040DmaControl controlAlias2;
  volatile uint32_t countAlias2;
  const volatile void *volatile sourceAlias2;
  const volatile void *volatile destinationTrigger;

  // Alias 3
  volatile Rp2040DmaControl controlAlias3;
  const volatile void *volatile destinationAlias3;
  volatile uint32_t countAlias3;
  const volatile void *volatile sourceTrigger;

  inline bool IsBusy() const { return control.busy; }

  inline void WaitUntilComplete() const {
    while (IsBusy()) {
    }
  }

  void Abort() { dmaAbort->Abort(this - (Rp2040Dma *)0x50000000); }
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
