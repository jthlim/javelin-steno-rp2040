//---------------------------------------------------------------------------

#include "rp2040_split.h"
#include "javelin/console.h"
#include "javelin/crc.h"
#include "rp2040_dma.h"
#include "rp2040_sniff.h"
#include "rp2040_spinlock.h"
#if JAVELIN_SPLIT_TX_PIN == JAVELIN_SPLIT_RX_PIN
#include "rp2040_split.pio.h"
#endif
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/timer.h>

//---------------------------------------------------------------------------

#if JAVELIN_SPLIT

//---------------------------------------------------------------------------

Rp2040Split::SplitData Rp2040Split::instance;

//---------------------------------------------------------------------------

#if !defined(JAVELIN_SPLIT_IS_LEFT)
bool Split::IsLeft() {
#if defined(JAVELIN_SPLIT_SIDE_PIN)
  return gpio_get(JAVELIN_SPLIT_SIDE_PIN);
#endif
  return false;
}
#endif

//---------------------------------------------------------------------------

const PIO PIO_INSTANCE = pio0;

#if JAVELIN_SPLIT_TX_PIN == JAVELIN_SPLIT_RX_PIN
const int TX_STATE_MACHINE_INDEX = 0;
const int RX_STATE_MACHINE_INDEX = 0;
#else
#error Not implemented yet
#endif

const uint32_t MASTER_RECEIVE_TIMEOUT_US = 2000;
const uint32_t SLAVE_RECEIVE_TIMEOUT_US = 50000;

void Rp2040Split::SplitData::StartTx() {
  const PIO pio = PIO_INSTANCE;
  const int sm = TX_STATE_MACHINE_INDEX;
  const int pin = JAVELIN_SPLIT_TX_PIN;

  pio_sm_set_enabled(pio, sm, false);
  pio_sm_set_pins_with_mask(pio, sm, 0, 1u << pin);
  pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
  pio_gpio_init(pio, pin);
#if JAVELIN_SPLIT_TX_PIN == JAVELIN_SPLIT_RX_PIN
  pio_sm_init(pio, sm, programOffset + rp2040split_offset_tx_start, &config);
#else
  pio_sm_init(pio, sm, programOffset + rp2040split_offset_tx_start, &txConfig);
#endif

  pio_sm_set_enabled(pio, sm, true);
}

Rp2040Split::SplitData::SplitData() {
  state = IsMaster() ? State::READY_TO_SEND : State::RECEIVING;
}

void Rp2040Split::SplitData::StartRx() {
  const PIO pio = PIO_INSTANCE;
  const int sm = RX_STATE_MACHINE_INDEX;
  const int pin = JAVELIN_SPLIT_RX_PIN;

  pio_sm_set_enabled(pio, sm, false);
  pio_sm_set_pins_with_mask(pio, sm, 0, 1u << pin);
  pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, false);
#if JAVELIN_SPLIT_TX_PIN == JAVELIN_SPLIT_RX_PIN
  pio_sm_init(pio, sm, programOffset + rp2040split_offset_rx_start, &config);
#else
  pio_sm_init(pio, sm, programOffset + rp2040split_offset_rx_start, &rxConfig);
#endif

  ResetRxDma();
  state = State::RECEIVING;
  receiveStartTime = time_us_32();

  pio_sm_set_enabled(pio, sm, true);
}

void Rp2040Split::SplitData::ResetRxDma() {
  dma3->Abort();
  dma3->source = &PIO_INSTANCE->rxf[RX_STATE_MACHINE_INDEX];
  dma3->destination = &rxBuffer.header;
  dma3->count =
      JAVELIN_SPLIT_TX_RX_BUFFER_SIZE + sizeof(TxRxHeader) / sizeof(uint32_t);

  Rp2040DmaControl receiveControl = {
      .enable = true,
      .dataSize = Rp2040DmaControl::DataSize::WORD,
      .incrementRead = false,
      .incrementWrite = true,
      .chainToDma = 3,
      .transferRequest = Rp2040DmaTransferRequest::PIO0_RX0,
      .sniffEnable = false,
  };
  dma3->controlTrigger = receiveControl;
}

void Rp2040Split::SplitData::Initialize() {
  programOffset = pio_add_program(PIO_INSTANCE, &rp2040split_program);

#if JAVELIN_SPLIT_TX_PIN == JAVELIN_SPLIT_RX_PIN
  config = rp2040split_program_get_default_config(programOffset);
  sm_config_set_in_pins(&config, JAVELIN_SPLIT_RX_PIN);
  sm_config_set_jmp_pin(&config, JAVELIN_SPLIT_RX_PIN);
  sm_config_set_out_pins(&config, JAVELIN_SPLIT_TX_PIN, 1);
  sm_config_set_set_pins(&config, JAVELIN_SPLIT_TX_PIN, 1);
  sm_config_set_sideset_pins(&config, JAVELIN_SPLIT_TX_PIN);
  sm_config_set_in_shift(&config, true, true, 32);
  sm_config_set_out_shift(&config, true, true, 32);
  sm_config_set_clkdiv_int_frac(&config, 1, 0);
#else
  rxConfig = rp2040split_program_get_default_config(programOffset);
  sm_config_set_in_pins(&rxConfig, JAVELIN_SPLIT_RX_PIN);
  sm_config_set_jmp_pin(&rxConfig, JAVELIN_SPLIT_RX_PIN);
  sm_config_set_in_shift(&rxConfig, true, true, 32);
  sm_config_set_clkdiv(&rxConfig, 1.0);

  txConfig = rp2040split_program_get_default_config(programOffset);
  sm_config_set_sideset_pins(&txConfig, JAVELIN_SPLIT_TX_PIN);
  sm_config_set_out_shift(&txConfig, true, true, 32);
  sm_config_set_fifo_join(&txConfig, PIO_FIFO_JOIN_TX);
  sm_config_set_in_pins(&txConfig, JAVELIN_SPLIT_TX_PIN);
  sm_config_set_jmp_pin(&txConfig, JAVELIN_SPLIT_TX_PIN);
  sm_config_set_in_shift(&txConfig, true, true, 32);
  sm_config_set_clkdiv(&txConfig, 1.0);
#endif

  irq_set_exclusive_handler(PIO0_IRQ_0, TxIrqHandler);
  irq_set_enabled(PIO0_IRQ_0, true);
  pio_set_irq0_source_enabled(PIO_INSTANCE, pis_interrupt0, true);

  gpio_pull_down(JAVELIN_SPLIT_RX_PIN);

  if (!IsMaster()) {
    StartRx();
  }
}

void Rp2040Split::SplitData::SendTxBuffer() {
  // Since Rx immediately follows Tx, set Rx dma before sending anything.
  ResetRxDma();

  txBuffer.header.crc =
      Crc32(txBuffer.buffer, sizeof(uint32_t) * txBuffer.header.wordCount);

  dma2->source = &txBuffer.header;
  dma2->destination = &PIO_INSTANCE->txf[TX_STATE_MACHINE_INDEX];
  size_t wordCount =
      txBuffer.header.wordCount + sizeof(TxRxHeader) / sizeof(uint32_t);
  txWords += wordCount;
  dma2->count = wordCount;

  size_t bitCount = 32 * wordCount;
  pio_sm_put_blocking(PIO_INSTANCE, TX_STATE_MACHINE_INDEX, bitCount - 1);

  Rp2040DmaControl sendControl = {
      .enable = true,
      .dataSize = Rp2040DmaControl::DataSize::WORD,
      .incrementRead = true,
      .incrementWrite = false,
      .chainToDma = 2,
      .transferRequest = Rp2040DmaTransferRequest::PIO0_TX0,
      .sniffEnable = false,
  };
  dma2->controlTrigger = sendControl;
}

void Rp2040Split::SplitData::ProcessReceiveBuffer() {
  size_t offset = 0;
  while (offset < rxBuffer.header.wordCount) {
    uint32_t blockHeader = rxBuffer.buffer[offset++];
    int type = blockHeader >> 16;
    size_t length = blockHeader & 0xffff;

    if (type < (size_t)SplitHandlerId::COUNT) {
      SplitRxHandler *handler = rxHandlers[type];
      if (handler != nullptr) {
        handler->OnDataReceived(&rxBuffer.buffer[offset], length);
      }
    }

    uint32_t wordLength = (length + 3) >> 2;
    offset += wordLength;
  }
}

void Rp2040Split::SplitData::OnReceiveFailed() {
  if (IsMaster()) {
    state = State::READY_TO_SEND;
  } else {
    StartRx();
  }
}

void Rp2040Split::SplitData::OnReceiveTimeout() {
  for (size_t i = 0; i < txHandlerCount; ++i) {
    txHandlers[i]->OnTransmitConnectionReset();
  }

  for (size_t i = 0; i < sizeof(rxHandlers) / sizeof(*rxHandlers); ++i) {
    SplitRxHandler *handler = rxHandlers[i];
    if (handler) {
      handler->OnReceiveConnectionReset();
    }
  }

  OnReceiveFailed();
}

void Rp2040Split::SplitData::OnReceiveSucceeded() {
  // Receiving succeeded without timeout means the previous transmit was
  // successful.
  for (size_t i = 0; i < txHandlerCount; ++i) {
    txHandlers[i]->OnTransmitSucceeded();
  }

  // After receiving data, immediately start sending the data here.
  SendData();
}

bool Rp2040Split::SplitData::ProcessReceive() {
  size_t dma3Count = dma3->count;
  if (dma3Count > JAVELIN_SPLIT_TX_RX_BUFFER_SIZE) {
    // Header has not been received.
    receiveStatusReason[0]++;
    return false;
  }

  if (rxBuffer.header.magic != TxRxHeader::MAGIC) {
    // Error: Magic mismatch!
    receiveStatusReason[3]++;
    OnReceiveFailed();
    return false;
  }

  size_t bufferCount = JAVELIN_SPLIT_TX_RX_BUFFER_SIZE - dma3Count;
  if (bufferCount < rxBuffer.header.wordCount) {
    // Data has not been fully received.
    receiveStatusReason[1]++;
    return false;
  }

  if (rxBuffer.header.wordCount != bufferCount) {
    // Excess data has been received.
    receiveStatusReason[5]++;
  }

  uint32_t expectedCrc =
      Crc32(rxBuffer.buffer, sizeof(uint32_t) * rxBuffer.header.wordCount);
  if (rxBuffer.header.crc != expectedCrc) {
    // Crc failure.
    receiveStatusReason[4]++;
    OnReceiveFailed();
    return false;
  }

  ++rxPacketCount;
  rxWords += rxBuffer.header.wordCount + sizeof(TxRxHeader) / sizeof(uint32_t);
  ProcessReceiveBuffer();

  OnReceiveSucceeded();

  return true;
}

void Rp2040Split::SplitData::SendData() {
  dma2->WaitUntilComplete();
  StartTx();
  state = State::SENDING;
  txBuffer.Reset();
  for (size_t i = 0; i < txHandlerCount; ++i) {
    txHandlers[i]->UpdateBuffer(txBuffer);
  }
  SendTxBuffer();
}

void Rp2040Split::SplitData::Update() {
  switch (state) {
  case State::READY_TO_SEND:
    SendData();
    break;

  case State::SENDING:
    // Do nothing, wait until TxIrqHandler happens.
    break;

  case State::RECEIVING:
    if (!ProcessReceive()) {
      uint32_t now = time_us_32();
      uint32_t timeSinceLastUpdate = now - receiveStartTime;
      uint32_t receiveTimeout =
          IsMaster() ? MASTER_RECEIVE_TIMEOUT_US : SLAVE_RECEIVE_TIMEOUT_US;
      if (timeSinceLastUpdate > receiveTimeout) {
        receiveStatusReason[2]++;
        OnReceiveTimeout();
      }
    }
    break;
  }
}

void Rp2040Split::SplitData::PrintInfo() {
  Console::Printf("Split data\n");
  Console::Printf("  Transmitted bytes/packets: %llu/%llu\n", 4 * txWords,
                  txIrqCount);
  Console::Printf("  Received bytes/packets: %llu/%llu\n", 4 * rxWords,
                  rxPacketCount);
  Console::Printf("  Transmit types:");
  for (size_t count : TxBuffer::txPacketTypeCounts) {
    Console::Printf(" %zu", count);
  }
  Console::Printf("\n");
  Console::Printf("  Receive status:");
  for (size_t reason : receiveStatusReason) {
    Console::Printf(" %zu", reason);
  }
  Console::Printf("\n");
}

void __no_inline_not_in_flash_func(Rp2040Split::SplitData::TxIrqHandler)() {
  pio_interrupt_clear(PIO_INSTANCE, 0);
  instance.txIrqCount++;
  instance.state = State::RECEIVING;
  instance.receiveStartTime = time_us_32();
}

//---------------------------------------------------------------------------

#endif // JAVELIN_SPLIT

//---------------------------------------------------------------------------
