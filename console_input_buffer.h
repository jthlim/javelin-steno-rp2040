//---------------------------------------------------------------------------

#pragma once
#include "javelin/queue.h"
#include "split_tx_rx.h"
#include <stdint.h>
#include <stdlib.h>

//---------------------------------------------------------------------------

class ConsoleInputBuffer {
public:
  static void Add(const uint8_t *data, size_t length) {
    instance.Add(data, length);
  };

  static void Process() { instance.Process(); }

  static void RegisterTxHandler() {
#if JAVELIN_SPLIT
    SplitTxRx::RegisterTxHandler(&instance);
#endif
  }

  static void RegisterRxHandler() {
#if JAVELIN_SPLIT
    SplitTxRx::RegisterRxHandler(SplitHandlerId::CONSOLE, &instance);
#endif
  }

private:
  struct EntryData {
    size_t length;
    char data[0];
  };

#if JAVELIN_SPLIT
  struct ConsoleInputBufferData : public Queue<EntryData>,
                                  public SplitTxHandler,
                                  SplitRxHandler {
#else
  struct ConsoleInputBufferData : public Queue<EntryData> {
#endif
    void Add(const uint8_t *data, size_t length);
    void Process();

    static QueueEntry<EntryData> *CreateEntry(const void *data, size_t length);

#if JAVELIN_SPLIT
    bool isConnected = false;

    virtual void UpdateBuffer(TxBuffer &buffer);
    virtual void OnDataReceived(const void *data, size_t length);
    virtual void OnTransmitConnectionReset() { isConnected = false; }
    virtual void OnTransmitSucceeded() { isConnected = true; }
#endif
  };

  static ConsoleInputBufferData instance;
};

//---------------------------------------------------------------------------
