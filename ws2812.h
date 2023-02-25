//---------------------------------------------------------------------------

#pragma once
#include "split_tx_rx.h"
#include JAVELIN_BOARD_CONFIG

//---------------------------------------------------------------------------

#if defined(JAVELIN_RGB_COUNT) && defined(JAVELIN_RGB_PIN)

class Ws2812 {
public:
  static void Initialize();
  static void Update() { instance.Update(); }
  static bool IsAvailable() { return true; }
  static size_t GetCount() { return JAVELIN_RGB_COUNT; }

  static void SetPixel(size_t pixelId, int r, int g, int b) {
    instance.SetPixel(pixelId, (r << 16) | (g << 24) | (b << 8));
  }
  static void SetPixel(size_t pixelId, uint32_t ws2812Color) {
    instance.SetPixel(pixelId, ws2812Color);
  }

  static void RegisterTxHandler() { SplitTxRx::RegisterTxHandler(&instance); }

  static void RegisterRxHandler() {
    SplitTxRx::RegisterRxHandler(SplitHandlerId::RGB, &instance);
  }

private:
#if JAVELIN_SPLIT
  struct Ws28128Data final : public SplitTxHandler, SplitRxHandler {
#else
  struct Ws28128Data {
#endif
    bool dirty;
#if JAVELIN_SPLIT
    bool slaveDirty;
#endif

    uint32_t lastUpdate;
    uint32_t pixelValues[JAVELIN_RGB_COUNT];

    void Update();
    void SetPixel(size_t pixelId, uint32_t ws2812Color) {
      if (pixelId >= JAVELIN_RGB_COUNT) {
        return;
      }
#if JAVELIN_USE_RGB_MAP
      pixelId = RGB_MAP[pixelId];
#endif
      if (pixelValues[pixelId] == ws2812Color) {
        return;
      }
#if JAVELIN_SPLIT
      if (pixelId < JAVELIN_RGB_MASTER_COUNT) {
        dirty = true;
      } else {
        slaveDirty = true;
      }
#else
      dirty = true;
#endif
      pixelValues[pixelId] = ws2812Color;
    }

#if JAVELIN_SPLIT
    virtual void UpdateBuffer(TxBuffer &buffer);
    virtual void OnTransmitConnectionReset() { slaveDirty = true; }
    virtual void OnDataReceived(const void *data, size_t length);
#endif
  };

  static Ws28128Data instance;
};

#else

// Do nothing instance.
class Ws2812 {
public:
  static void Initialize() {}
  static void Update() {}
  static bool IsAvailable() { return false; }
  static size_t GetCount() { return 0; }

  static void SetPixel(size_t pixelId, int r, int g, int b) {}
  static void SetPixel(size_t pixelId, uint32_t ws2812Color) {}

  static void RegisterTxHandler() {}
  static void RegisterRxHandler() {}
};

#endif

//---------------------------------------------------------------------------
