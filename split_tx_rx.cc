//---------------------------------------------------------------------------

#include "split_tx_rx.h"
#include "javelin/console.h"
#include "javelin/crc32.h"
#include "rp2040_dma.h"
#include "rp2040_sniff.h"
#include "rp2040_spinlock.h"
#include "split_tx_rx.pio.h"
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

const PIO PIO_INSTANCE = pio1;

#if JAVELIN_SPLIT_TX_PIN == JAVELIN_SPLIT_RX_PIN
const int TX_STATE_MACHINE_INDEX = 0;
const int RX_STATE_MACHINE_INDEX = 0;
#else
#error Not implemented yet
#endif

const uint32_t SYNC_DATA = 0;
const uint32_t MAGIC = 0x5352534a; // 'JSRS';

const uint32_t MASTER_RECEIVE_TIMEOUT_US = 2000;
const uint32_t SLAVE_RECEIVE_TIMEOUT_US = 50000;

void SplitTxRx::SplitTxRxData::StartTx() {
  const PIO pio = PIO_INSTANCE;
  const int sm = TX_STATE_MACHINE_INDEX;
  const int pin = JAVELIN_SPLIT_TX_PIN;

  pio_sm_set_enabled(pio, sm, false);
  pio_sm_set_pins_with_mask(pio, sm, 0, 1u << pin);
  pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
  pio_gpio_init(pio, pin);
#if JAVELIN_SPLIT_TX_PIN == JAVELIN_SPLIT_RX_PIN
  pio_sm_init(pio, sm, programOffset + split_tx_rx_offset_tx_start, &config);
#else
  pio_sm_init(pio, sm, programOffset + split_tx_rx_offset_tx_start, &txConfig);
#endif

  pio_sm_set_enabled(pio, sm, true);
}

SplitTxRx::SplitTxRxData::SplitTxRxData() {
  state = IsMaster() ? State::READY_TO_SEND : State::RECEIVING;
}

bool SplitTxRx::IsMaster() { return gpio_get(JAVELIN_SPLIT_SIDE_PIN); }

void SplitTxRx::SplitTxRxData::StartRx() {
  const PIO pio = PIO_INSTANCE;
  const int sm = RX_STATE_MACHINE_INDEX;
  const int pin = JAVELIN_SPLIT_RX_PIN;

  pio_sm_set_enabled(pio, sm, false);
  pio_sm_set_pins_with_mask(pio, sm, 0, 1u << pin);
  pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, false);
#if JAVELIN_SPLIT_TX_PIN == JAVELIN_SPLIT_RX_PIN
  pio_sm_init(pio, sm, programOffset + split_tx_rx_offset_rx_start, &config);
#else
  pio_sm_init(pio, sm, programOffset + split_tx_rx_offset_rx_start, &rxConfig);
#endif
  pio_sm_set_enabled(pio, sm, true);

  ResetRxDma();
  state = State::RECEIVING;
  receiveStartTime = time_us_32();
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
      .transferRequest = Rp2040DmaControl::TransferRequest::PIO1_RX0,
      .sniffEnable = false,
  };
  dma3->controlTrigger = receiveControl;
}

void SplitTxRx::SplitTxRxData::Initialize() {
  programOffset = pio_add_program(PIO_INSTANCE, &split_tx_rx_program);

#if JAVELIN_SPLIT_TX_PIN == JAVELIN_SPLIT_RX_PIN
  config = split_tx_rx_program_get_default_config(programOffset);
  sm_config_set_in_pins(&config, JAVELIN_SPLIT_RX_PIN);
  sm_config_set_jmp_pin(&config, JAVELIN_SPLIT_RX_PIN);
  sm_config_set_out_pins(&config, JAVELIN_SPLIT_TX_PIN, 1);
  sm_config_set_set_pins(&config, JAVELIN_SPLIT_TX_PIN, 1);
  sm_config_set_sideset_pins(&config, JAVELIN_SPLIT_TX_PIN);
  sm_config_set_in_shift(&config, true, true, 32);
  sm_config_set_out_shift(&config, true, true, 32);
  sm_config_set_clkdiv_int_frac(&config, 1, 0);
#else
  rxConfig = split_tx_rx_program_get_default_config(programOffset);
  sm_config_set_in_pins(&rxConfig, JAVELIN_SPLIT_RX_PIN);
  sm_config_set_jmp_pin(&rxConfig, JAVELIN_SPLIT_RX_PIN);
  sm_config_set_in_shift(&rxConfig, true, true, 32);
  sm_config_set_clkdiv(&rxConfig, 1.0);

  txConfig = split_tx_rx_program_get_default_config(programOffset);
  sm_config_set_sideset_pins(&txConfig, JAVELIN_SPLIT_TX_PIN);
  sm_config_set_out_shift(&txConfig, true, true, 32);
  sm_config_set_fifo_join(&txConfig, PIO_FIFO_JOIN_TX);
  sm_config_set_in_pins(&txConfig, JAVELIN_SPLIT_TX_PIN);
  sm_config_set_jmp_pin(&txConfig, JAVELIN_SPLIT_TX_PIN);
  sm_config_set_in_shift(&txConfig, true, true, 32);
  sm_config_set_clkdiv(&txConfig, 1.0);
#endif

  irq_set_exclusive_handler(PIO1_IRQ_0, TxIrqHandler);
  irq_set_enabled(PIO1_IRQ_0, true);
  pio_set_irq0_source_enabled(PIO_INSTANCE, pis_interrupt0, true);

  gpio_pull_down(JAVELIN_SPLIT_RX_PIN);

  if (IsMaster()) {
    // TODO: Anything?
  } else {
    StartRx();
  }
}

void SplitTxRx::SplitTxRxData::SendTxBuffer() {
  // Since Rx immediately follows Tx, set Rx dma before sending anything.
  ResetRxDma();

  txBuffer.header.syncData = SYNC_DATA;
  txBuffer.header.magic = MAGIC;
  txBuffer.header.crc =
      Crc32(txBuffer.buffer, sizeof(uint32_t) * txBuffer.header.count);

  dma2->source = &txBuffer.header;
  dma2->destination = &PIO_INSTANCE->txf[TX_STATE_MACHINE_INDEX];
  size_t wordCount =
      txBuffer.header.count + sizeof(TxRxHeader) / sizeof(uint32_t);
  dma2->count = wordCount;

  size_t bitCount = 32 * wordCount;
  pio_sm_put_blocking(PIO_INSTANCE, TX_STATE_MACHINE_INDEX, bitCount - 1);

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

void SplitTxRx::SplitTxRxData::OnReceiveFailed() {
  if (IsMaster()) {
    state = State::READY_TO_SEND;
  } else {
    StartRx();
  }
}

void SplitTxRx::SplitTxRxData::OnReceiveEnd() { state = State::READY_TO_SEND; }

void SplitTxRx::SplitTxRxData::OnReceiveTimeout() {
  if (IsMaster()) {
    // TODO: Run this after ~1 sec.
    // for (size_t i = 0; i < sizeof(rxHandlers) / sizeof(*rxHandlers); ++i) {
    //   SplitRxHandler *handler = rxHandlers[i];
    //   if (handler) {
    //     handler->OnConnectionReset();
    //   }
    // }
  }
  OnReceiveFailed();
}

void SplitTxRx::SplitTxRxData::ProcessReceive() {
  size_t dma3Count = dma3->count;
  if (dma3Count > BUFFER_SIZE) {
    // Header has not been received.
    receiveStatusReason[1]++;
    return;
  }

  if (rxBuffer.header.magic != MAGIC) {
    // if (time_us_32() > 3000000) {
    //   Console::Printf("Bad magic: %08x, crc: %08x\n\n",
    //   rxBuffer.header.magic,
    //                   rxBuffer.header.crc);
    // }
    receiveStatusReason[2]++;
    //    OnReceiveFailed();
    OnReceiveEnd();
    return;
  }

  size_t bufferCount = BUFFER_SIZE - dma3Count;
  if (bufferCount < rxBuffer.header.count) {
    receiveStatusReason[3]++;
    return;
  }

  if (rxBuffer.header.count != bufferCount) {
    // if (time_us_32() > 3000000) {
    //   Console::Printf("Extra data: %zu vs %u\n\n", bufferCount,
    //                   rxBuffer.header.count);
    // }
    receiveStatusReason[4]++;
  }

  uint32_t expectedCrc =
      Crc32(rxBuffer.buffer, sizeof(uint32_t) * rxBuffer.header.count);
  if (rxBuffer.header.crc != expectedCrc) {
    // if (time_us_32() > 3000000) {
    //   Console::Printf("Bad CRC: %08x vs %08x, %u\n", rxBuffer.header.crc,
    //                   expectedCrc, rxBuffer.header.count);
    //   for (int i = 0; i < rxBuffer.header.count; ++i) {
    //     Console::Printf(" %08x", rxBuffer.buffer[i]);
    //   }
    //   Console::Printf("\n\n");
    // }
    receiveStatusReason[5]++;
    //    OnReceiveFailed();
    OnReceiveEnd();
    return;
  }

  ++rxPacketCount;
  ProcessReceiveBuffer();
  OnReceiveEnd();
}

void SplitTxRx::SplitTxRxData::SendData() {
  dma2->WaitUntilComplete();
  StartTx();
  state = State::SENDING;
  txBuffer.Reset();
  for (size_t i = 0; i < txHandlerCount; ++i) {
    txHandlers[i]->UpdateBuffer(txBuffer);
  }
  SendTxBuffer();
}

void SplitTxRx::SplitTxRxData::Update() {
  switch (state) {
  case State::READY_TO_SEND:
    SendData();
    break;

  case State::SENDING:
    // Do nothing, wait until TxIRQ happens.
    break;

  case State::RECEIVING:
    uint32_t now = time_us_32();
    uint32_t timeSinceLastUpdate = now - receiveStartTime;
    uint32_t receiveTimeout =
        IsMaster() ? MASTER_RECEIVE_TIMEOUT_US : SLAVE_RECEIVE_TIMEOUT_US;
    if (timeSinceLastUpdate > receiveTimeout) {
      receiveStatusReason[0]++;
      OnReceiveTimeout();
      return;
    }

    ProcessReceive();
  }
}

void SplitTxRx::SplitTxRxData::PrintInfo() {
  Console::Printf("Split data\n");
  Console::Printf("  Transmitted packets: %zu\n", txIrqCount);
  Console::Printf("  Received packets: %zu\n", rxPacketCount);
  Console::Printf("  Receive status:");
  for (size_t reason : receiveStatusReason) {
    Console::Printf(" %zu", reason);
  }
  Console::Printf("\n");
}

void __no_inline_not_in_flash_func(SplitTxRx::SplitTxRxData::TxIrqHandler)() {
  pio_interrupt_clear(PIO_INSTANCE, 0);
  instance.txIrqCount++;
  instance.state = State::RECEIVING;
  instance.receiveStartTime = time_us_32();
}

//---------------------------------------------------------------------------

#endif // JAVELIN_SPLIT

//---------------------------------------------------------------------------