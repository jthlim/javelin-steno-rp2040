//---------------------------------------------------------------------------

#pragma once
#include "split_tx_rx.h"
#include <stdint.h>
#include <stdlib.h>

//---------------------------------------------------------------------------

#if JAVELIN_SPLIT

class SplitLedStatus {
public:
  static void Set(uint8_t value) { instance.Set(value); }

  static void RegisterTxHandler() { SplitTxRx::RegisterTxHandler(&instance); }

  static void RegisterRxHandler() {
    SplitTxRx::RegisterRxHandler(SplitHandlerId::LED_STATUS, &instance);
  }

private:
  struct SplitLedStatusData : public SplitTxHandler, public SplitRxHandler {
    bool transmitting = false;
    bool dirty = false;
    uint8_t value;

    void Set(uint8_t newValue) {
      dirty = true;
      value = newValue;
    }

    virtual void UpdateBuffer(TxBuffer &buffer);
    virtual void OnTransmitSucceeded();
    virtual void OnDataReceived(const void *data, size_t length);
  };

  static SplitLedStatusData instance;
};

#else

class SplitLedStatus {
public:
  static void Set(uint8_t value) {}

  static void RegisterTxHandler() {}
  static void RegisterRxHandler() {}
};

#endif // JAVELIN_SPLIT

//---------------------------------------------------------------------------
