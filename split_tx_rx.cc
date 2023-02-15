//---------------------------------------------------------------------------

#include "split_tx_rx.h"
#include "javelin/console.h"
#include "javelin/crc32.h"
#include "rp2040_dma.h"
#include "rp2040_sniff.h"
#include "rp2040_spinlock.h"
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/timer.h>

//---------------------------------------------------------------------------

void TxBuffer::Add(SplitHandlerId id, const void *data, size_t length) {
  uint32_t wordLength = (length + 3) >> 2;
  if (header.count + 1 + wordLength > BUFFER_SIZE) {
    return;
  }

  uint32_t blockHeader = (id << 16) | length;
  buffer[header.count++] = blockHeader;

  memcpy(&buffer[header.count], data, length);
  header.count += wordLength;
}

//---------------------------------------------------------------------------

#if JAVELIN_SPLIT

//---------------------------------------------------------------------------

SplitTxRx::SplitTxRxData SplitTxRx::instance;

//---------------------------------------------------------------------------

const int differential_manchester_tx_wrap_target = 0;
const int differential_manchester_tx_wrap = 7;
const int differential_manchester_tx_offset_start = 0;

const int differential_manchester_rx_wrap_target = 5;
const int differential_manchester_rx_wrap = 9;
const int differential_manchester_rx_offset_start = 0;

const PIO PIO_INSTANCE = pio1;
const int TX_STATE_MACHINE_INDEX = 0;
const int RX_STATE_MACHINE_INDEX = 1;

const uint32_t SYNC_DATA = 0;
const uint32_t MAGIC = 0x5352534a; // 'JSRS';

// clang-format off
static const uint16_t TX_PROGRAM_INSTRUCTIONS[] = {
            //     .wrap_target
    0x6021, //  0: out    x, 1
    0x1c23, //  1: jmp    !x, 3           side 1 [4]
    0x1300, //  2: jmp    0               side 0 [3]
    0x0304, //  3: jmp    4                      [3]
    0x6021, //  4: out    x, 1
    0x1427, //  5: jmp    !x, 7           side 0 [4]
    0x1b04, //  6: jmp    4               side 1 [3]
    0x0300, //  7: jmp    0                      [3]
            //     .wrap
};

static const uint16_t RX_PROGRAM_INSTRUCTIONS[] = {
    0x25a0, //  0: wait   1 pin, 0               [5]
    0x00c4, //  1: jmp    pin, 4
    0x4021, //  2: in     x, 1
    0x0000, //  3: jmp    0
    0x4141, //  4: in     y, 1                   [1]
            //     .wrap_target
    0x2520, //  5: wait   0 pin, 0               [5]
    0x00c9, //  6: jmp    pin, 9
    0x4041, //  7: in     y, 1
    0x0000, //  8: jmp    0
    0x4121, //  9: in     x, 1                   [1]
            //     .wrap
};
// clang-format on

static const struct pio_program TX_PROGRAM = {
    .instructions = TX_PROGRAM_INSTRUCTIONS,
    .length = 10,
    .origin = -1,
};

static const struct pio_program RX_PROGRAM = {
    .instructions = RX_PROGRAM_INSTRUCTIONS,
    .length = 10,
    .origin = -1,
};

static inline pio_sm_config
differential_manchester_tx_program_get_default_config(uint offset) {
  pio_sm_config c = pio_get_default_sm_config();
  sm_config_set_wrap(&c, offset + differential_manchester_tx_wrap_target,
                     offset + differential_manchester_tx_wrap);
  sm_config_set_sideset(&c, 2, true, false);
  return c;
}

void SplitTxRx::SplitTxRxData::InitializeTx(uint32_t offset) {
  const PIO pio = PIO_INSTANCE;
  const int sm = TX_STATE_MACHINE_INDEX;
  const int pin = JAVELIN_SPLIT_TX_PIN;

  pio_sm_set_pins_with_mask(pio, sm, 0, 1u << pin);
  pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
  pio_gpio_init(pio, pin);
  pio_sm_config c =
      differential_manchester_tx_program_get_default_config(offset);
  sm_config_set_sideset_pins(&c, pin);
  sm_config_set_out_shift(&c, true, true, 32);
  sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
  sm_config_set_clkdiv(&c, 1.0);
  pio_sm_init(pio, sm, offset + differential_manchester_tx_offset_start, &c);
  // Execute a blocking pull so that we maintain the initial line state until
  // data is available
  pio_sm_exec(pio, sm, pio_encode_pull(false, true));
  pio_sm_set_enabled(pio, sm, true);
}

static inline pio_sm_config
differential_manchester_rx_program_get_default_config(uint32_t offset) {
  pio_sm_config c = pio_get_default_sm_config();
  sm_config_set_wrap(&c, offset + differential_manchester_rx_wrap_target,
                     offset + differential_manchester_rx_wrap);
  return c;
}

SplitTxRx::SplitTxRxData::SplitTxRxData() { isSending = !IsMaster(); }

bool SplitTxRx::IsMaster() { return gpio_get(JAVELIN_SPLIT_SIDE_PIN); }

void SplitTxRx::SplitTxRxData::InitializeRx(uint32_t offset) {
  const PIO pio = PIO_INSTANCE;
  const int sm = RX_STATE_MACHINE_INDEX;
  const int pin = JAVELIN_SPLIT_RX_PIN;

  pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, false);
  pio_gpio_init(pio, pin);
  pio_sm_config c =
      differential_manchester_rx_program_get_default_config(offset);
  sm_config_set_in_pins(&c, pin); // for WAIT
  sm_config_set_jmp_pin(&c, pin); // for JMP
  sm_config_set_in_shift(&c, true, true, 32);
  sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
  sm_config_set_clkdiv(&c, 1.0);
  pio_sm_init(pio, sm, offset, &c);
  // X and Y are set to 0 and 1, to conveniently emit these to ISR/FIFO.
  pio_sm_exec(pio, sm, pio_encode_set(pio_x, 1));
  pio_sm_exec(pio, sm, pio_encode_set(pio_y, 0));
  pio_sm_set_enabled(pio, sm, true);

  ResetRxDma();
}

void SplitTxRx::SplitTxRxData::ResetRxDma() {
  dma3->Abort();
  dma3->source = &PIO_INSTANCE->rxf[RX_STATE_MACHINE_INDEX];
  dma3->destination = &rxBuffer.header;
  dma3->count = BUFFER_SIZE + sizeof(TxRxHeader) / sizeof(uint32_t);

  Rp2040DmaControl receiveControl = {
      .enable = true,
      .dataSize = Rp2040DmaControl::DataSize::WORD,
      .incrementRead = false,
      .incrementWrite = true,
      .chainToDma = 3,
      .transferRequest = Rp2040DmaControl::TransferRequest::PIO1_RX1,
      .sniffEnable = false,
  };
  dma3->controlTrigger = receiveControl;
}

void SplitTxRx::SplitTxRxData::Initialize() {

  if (isSending) {
    uint32_t txOffset = pio_add_program(PIO_INSTANCE, &TX_PROGRAM);
    InitializeTx(txOffset);
  } else {
    uint32_t rxOffset = pio_add_program(PIO_INSTANCE, &RX_PROGRAM);
    InitializeRx(rxOffset);
  }
}

void SplitTxRx::SplitTxRxData::SendTxBuffer() {
  txBuffer.header.syncData = SYNC_DATA;
  txBuffer.header.magic = MAGIC;
  txBuffer.header.crc =
      Crc32(txBuffer.buffer, sizeof(uint32_t) * txBuffer.header.count);

  dma2->source = &txBuffer.header;
  dma2->destination = &PIO_INSTANCE->txf[TX_STATE_MACHINE_INDEX];
  dma2->count = txBuffer.header.count + sizeof(TxRxHeader) / sizeof(uint32_t);

  Rp2040DmaControl sendControl = {
      .enable = true,
      .dataSize = Rp2040DmaControl::DataSize::WORD,
      .incrementRead = true,
      .incrementWrite = false,
      .chainToDma = 2,
      .transferRequest = Rp2040DmaControl::TransferRequest::PIO1_TX0,
      .sniffEnable = false,
  };
  dma2->controlTrigger = sendControl;
}

void SplitTxRx::SplitTxRxData::ProcessReceiveBuffer() {
  size_t offset = 0;
  while (offset < rxBuffer.header.count) {
    uint32_t blockHeader = rxBuffer.buffer[offset++];
    int type = blockHeader >> 16;
    size_t length = blockHeader & 0xffff;

    if (type < 8) {
      SplitRxHandler *handler = rxHandlers[type];
      if (handler != nullptr) {
        handler->OnDataReceived(&rxBuffer.buffer[offset], length);
      }
    }

    uint32_t wordLength = (length + 3) >> 2;
    offset += wordLength;
  }
}

void SplitTxRx::SplitTxRxData::ProcessSend() {
  uint32_t now = time_us_32();
  uint32_t timeSinceLastUpdate = now - lastSendTime;
  if (timeSinceLastUpdate < 1000) {
    return;
  }
  lastSendTime = now;

  dma2->WaitUntilComplete();

  txBuffer.Reset();
  for (size_t i = 0; i < txHandlerCount; ++i) {
    txHandlers[i]->UpdateBuffer(txBuffer);
  }
  SendTxBuffer();
}

void SplitTxRx::SplitTxRxData::ProcessReceive() {
  uint32_t now = time_us_32();
  uint32_t timeSinceLastUpdate = now - lastReceiveTime;
  if (timeSinceLastUpdate > 100000) {
    // 100ms has passed without receiving anything -> reset.
    lastReceiveTime = now;

    pio_sm_restart(PIO_INSTANCE, RX_STATE_MACHINE_INDEX);
    ResetRxDma();
    return;
  }

  size_t dma3Count = dma3->count;
  if (dma3Count > BUFFER_SIZE) {
    // Header has not been received.
    return;
  }

  if (rxBuffer.header.magic != MAGIC) {
    if (time_us_32() > 3000000) {
      Console::Printf("Bad magic: %08x, crc: %08x\n\n", rxBuffer.header.magic,
                      rxBuffer.header.crc);
    }
    ResetRxDma();
    return;
  }

  size_t bufferCount = BUFFER_SIZE - dma3Count;
  if (bufferCount < rxBuffer.header.count) {
    return;
  }

  if (rxBuffer.header.count != bufferCount) {
    if (time_us_32() > 3000000) {
      Console::Printf("Extra data: %zu vs %u\n\n", bufferCount,
                      rxBuffer.header.count);
    }
  }

  uint32_t expectedCrc =
      Crc32(rxBuffer.buffer, sizeof(uint32_t) * rxBuffer.header.count);
  if (rxBuffer.header.crc != expectedCrc) {
    if (time_us_32() > 3000000) {
      Console::Printf("Bad CRC: %08x vs %08x, %u\n", rxBuffer.header.crc,
                      expectedCrc, rxBuffer.header.count);
      for (int i = 0; i < rxBuffer.header.count; ++i) {
        Console::Printf(" %08x", rxBuffer.buffer[i]);
      }
      Console::Printf("\n\n");
    }
    ResetRxDma();
    return;
  }
  lastReceiveTime = now;
  ProcessReceiveBuffer();
  ResetRxDma();
}

void SplitTxRx::SplitTxRxData::Update() {
  if (isSending) {
    ProcessSend();
  } else {
    ProcessReceive();
  }
}

//---------------------------------------------------------------------------

#endif // JAVELIN_SPLIT

//---------------------------------------------------------------------------
