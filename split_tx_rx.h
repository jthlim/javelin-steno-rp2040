//---------------------------------------------------------------------------

#include JAVELIN_BOARD_CONFIG
#include <stdint.h>
#include <stdlib.h>

//---------------------------------------------------------------------------

enum SplitHandlerId {
  KEY_STATE,
};

struct TxRxHeader {
  uint32_t syncData;
  uint32_t magic;
  uint32_t count;
  uint32_t crc;
};

class TxBuffer {
public:
  void Reset() { header.count = 0; }
  void Add(SplitHandlerId id, const void *data, size_t length);

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
  virtual void UpdateBuffer(TxBuffer &buffer);
};

class SplitRxHandler {
public:
  virtual void OnConnectionReset() {}
  virtual void OnDataReceived(const void *data, size_t length);
};

#if JAVELIN_SPLIT

class SplitTxRx {
public:
  static void Initialize() { instance.Initialize(); }
  static bool IsMaster();

  static void Update() { instance.Update(); }

  static void RegisterTxHandler(SplitTxHandler *handler) {
    instance.txHandlers[instance.txHandlerCount++] = handler;
  }

  static void RegisterRxHandler(SplitHandlerId id, SplitRxHandler *handler) {
    instance.rxHandlers[id] = handler;
  }

private:
  struct SplitTxRxData {
    SplitTxRxData();

    static const size_t BUFFER_SIZE = 2048;

    bool isSending;
    uint32_t lastSendTime;
    uint32_t lastReceiveTime;

    size_t txHandlerCount = 0;
    SplitTxHandler *txHandlers[8];
    SplitRxHandler *rxHandlers[8];

    TxBuffer txBuffer;
    RxBuffer rxBuffer;

    void Initialize();
    void InitializeTx(uint32_t offset);
    void InitializeRx(uint32_t offset);

    void SendTxBuffer();
    void Update();
    void ResetRxDma();
    void ProcessReceiveBuffer();

    void ProcessSend();
    void ProcessReceive();
  };

  static SplitTxRxData instance;
};

#else

class SplitTxRx {
public:
  static void Initialize() {}
  static bool IsMaster() { return true; }
  static void Update() {}

  static void RegisterTxHandler(void *handler) {}
  static void RegisterRxHandler(SplitHandlerId id, void *handler) {}
};

#endif // JAVELIN_SPLIT

//---------------------------------------------------------------------------