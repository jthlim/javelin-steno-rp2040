//---------------------------------------------------------------------------

#pragma once
#include "javelin/split/split.h"
#include <hardware/pio.h>

//---------------------------------------------------------------------------

#if JAVELIN_SPLIT

class Rp2040Split : public Split {
public:
  static void Initialize() { instance.Initialize(); }

  static void Update() { instance.Update(); }

  static void RegisterTxHandler(SplitTxHandler *handler) {
    instance.txHandlers[instance.txHandlerCount++] = handler;
  }

  static void RegisterRxHandler(SplitHandlerId id, SplitRxHandler *handler) {
    instance.rxHandlers[(size_t)id] = handler;
  }

  static void PrintInfo() { instance.PrintInfo(); }

private:
  struct SplitData {
    enum class State {
      READY_TO_SEND,
      SENDING,
      RECEIVING,
    };

    SplitData();

    State state;
    uint32_t programOffset;
    uint32_t receiveStartTime;
    uint64_t rxPacketCount;
    uint64_t txIrqCount;
    uint64_t rxWords;
    uint64_t txWords;
    size_t receiveStatusReason[6];

    size_t txHandlerCount = 0;
    SplitTxHandler *txHandlers[(size_t)SplitHandlerId::COUNT];
    SplitRxHandler *rxHandlers[(size_t)SplitHandlerId::COUNT];

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

  static SplitData instance;
};

inline void Split::RegisterTxHandler(SplitTxHandler *handler) {
  Rp2040Split::RegisterTxHandler(handler);
}
inline void Split::RegisterRxHandler(SplitHandlerId id,
                                     SplitRxHandler *handler) {
  Rp2040Split::RegisterRxHandler(id, handler);
}

#else

class Rp2040Split : public Split {
public:
  static void Initialize() {}
  static void Update() {}
  static void PrintInfo() {}
};

inline void Split::RegisterTxHandler(void *handler) {}
inline void Split::RegisterRxHandler(SplitHandlerId id, void *handler) {}

#endif // JAVELIN_SPLIT

//---------------------------------------------------------------------------