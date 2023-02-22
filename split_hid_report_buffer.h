//---------------------------------------------------------------------------

#pragma once
#include "javelin/queue.h"
#include "split_tx_rx.h"
#include <stdint.h>
#include <stdlib.h>

//---------------------------------------------------------------------------

#if JAVELIN_SPLIT

class SplitHidReportBuffer {
public:
  static void Add(uint8_t interface, uint8_t reportId, const uint8_t *data,
                  size_t length) {
    instance.Add(interface, reportId, data, length);
  }

  static void Update() { instance.Update(); }

  static void RegisterTxHandler() {
    SplitTxRx::RegisterTxHandler(&instance);
    SplitTxRx::RegisterRxHandler(SplitHandlerId::HID_BUFFER_SIZE,
                                 &instance.bufferSize);
  }

  static void RegisterRxHandler() {
    SplitTxRx::RegisterRxHandler(SplitHandlerId::HID_REPORT, &instance);
    SplitTxRx::RegisterTxHandler(&instance.bufferSize);
  }

private:
  // To avoid flooding the HID buffers and exhausting memory, the buffer size
  // levels are shared and the sender will block until there is available space.
  union HidBufferSize {
    uint8_t available[4];
    uint32_t value;
  };

  struct SplitHidReportBufferSize : SplitTxHandler, SplitRxHandler {
    bool dirty;
    HidBufferSize bufferSize;

    virtual void UpdateBuffer(TxBuffer &buffer);
    virtual void OnDataReceived(const void *data, size_t length);
    virtual void OnConnectionReset() { dirty = true; }
  };

  struct EntryData {
    uint8_t interface;
    uint8_t reportId;
    uint8_t length;
    uint8_t data[0];
  };

  struct SplitHidReportBufferData : public Queue<EntryData>,
                                    SplitTxHandler,
                                    SplitRxHandler {
    SplitHidReportBufferSize bufferSize;

    void Add(uint8_t interface, uint8_t reportId, const uint8_t *data,
             size_t length);
    void Update();

    bool ProcessEntry(const QueueEntry<EntryData> *entry);

    static QueueEntry<EntryData> *CreateEntry(uint8_t interface,
                                              uint8_t reportId,
                                              const uint8_t *data,
                                              size_t length);

    virtual void UpdateBuffer(TxBuffer &buffer);
    virtual void OnDataReceived(const void *data, size_t length);
  };

  static SplitHidReportBufferData instance;
};

#else

class SplitHidReportBuffer {
public:
  static void Add(const uint8_t *data) {}
  static void Update() {}

  static void RegisterTxHandler() {}
  static void RegisterRxHandler() {}
};

#endif // JAVELIN_SPLIT

//---------------------------------------------------------------------------
