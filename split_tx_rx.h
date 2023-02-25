//---------------------------------------------------------------------------

#pragma once
#include JAVELIN_BOARD_CONFIG
#include <hardware/pio.h>
#include <stdint.h>
#include <stdlib.h>

//---------------------------------------------------------------------------

enum SplitHandlerId {
  KEY_STATE,
  RGB,
  HID_REPORT,
  CONSOLE,
  BOOTROM,
  LED_STATUS,
  SERIAL,
  HID_BUFFER_SIZE,
  OLED_DATA,
  OLED_AVAILABLE,

  COUNT,
};

struct TxRxHeader {
  uint16_t magic;
  uint16_t wordCount;
  uint32_t crc;

  static const uint16_t MAGIC = 0x534a; // 'JS';
};

class TxBuffer {
public:
  TxBuffer() { header.magic = TxRxHeader::MAGIC; }

  void Reset() { header.wordCount = 0; }
  bool Add(SplitHandlerId id, const void *data, size_t length);

  static const size_t BUFFER_SIZE = 2048;

  TxRxHeader header;
  uint32_t buffer[BUFFER_SIZE];
};

struct RxBuffer {
  static const size_t BUFFER_SIZE = 2048;

  TxRxHeader header;
  uint32_t buffer[BUFFER_SIZE];
};

class SplitTxHandler {
public:
  virtual void OnTransmitConnectionReset() {}
  virtual void OnTransmitSucceeded() {}
  virtual void UpdateBuffer(TxBuffer &buffer);
};

class SplitRxHandler {
public:
  virtual void OnReceiveConnectionReset() {}
  virtual void OnDataReceived(const void *data, size_t length);
};

#if JAVELIN_SPLIT

class SplitTxRx {
public:
  static void Initialize() { instance.Initialize(); }
  static bool IsMaster();
  static bool IsSlave() { return !IsMaster(); }

  static void Update() { instance.Update(); }

  static void RegisterTxHandler(SplitTxHandler *handler) {
    instance.txHandlers[instance.txHandlerCount++] = handler;
  }

  static void RegisterRxHandler(SplitHandlerId id, SplitRxHandler *handler) {
    instance.rxHandlers[id] = handler;
  }

  static void PrintInfo() { instance.PrintInfo(); }

private:
  struct SplitTxRxData {
    enum class State {
      READY_TO_SEND,
      SENDING,
      RECEIVING,
    };

    SplitTxRxData();

    static const size_t BUFFER_SIZE = 2048;

    State state;
    uint32_t programOffset;
    uint32_t receiveStartTime;
    size_t rxPacketCount;
    size_t txIrqCount;
    size_t receiveStatusReason[6];

    size_t txHandlerCount = 0;
    SplitTxHandler *txHandlers[SplitHandlerId::COUNT];
    SplitRxHandler *rxHandlers[SplitHandlerId::COUNT];

    TxBuffer txBuffer;
    RxBuffer rxBuffer;

#if JAVELIN_SPLIT_TX_PIN == JAVELIN_SPLIT_RX_PIN
    pio_sm_config config;
#endif

    void Initialize();
    void StartTx();
    void StartRx();

    void SendData();
    void SendTxBuffer();
    void Update();
    void ResetRxDma();
    void OnReceiveFailed();
    void OnReceiveTimeout();
    void OnReceiveSucceeded();
    void ProcessReceiveBuffer();

    bool ProcessReceive();

    static void TxIrqHandler();

    void PrintInfo();
  };

  static SplitTxRxData instance;
};

#else

class SplitTxRx {
public:
  static void Initialize() {}
  static bool IsMaster() { return true; }
  static void Update() {}

  static void PrintInfo() {}

  static void RegisterTxHandler(void *handler) {}
  static void RegisterRxHandler(SplitHandlerId id, void *handler) {}
};

#endif // JAVELIN_SPLIT

//---------------------------------------------------------------------------