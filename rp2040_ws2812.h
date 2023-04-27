//---------------------------------------------------------------------------

#pragma once
#include "javelin/split/split.h"
#include JAVELIN_BOARD_CONFIG

//---------------------------------------------------------------------------

#if JAVELIN_RGB

class Ws2812 {
public:
  static void Initialize();
  static void Update() { instance.Update(); }
  static bool IsAvailable() { return true; }
  static size_t GetCount() { return JAVELIN_RGB_COUNT; }

  static void SetRgb(size_t pixelId, int r, int g, int b) {
    instance.SetRgb(pixelId, (r << 16) | (g << 24) | (b << 8));
  }
  static void SetRgb(size_t pixelId, uint32_t ws2812Color) {
    instance.SetRgb(pixelId, ws2812Color);
  }

  static void RegisterTxHandler() { Split::RegisterTxHandler(&instance); }

  static void RegisterRxHandler() {
    Split::RegisterRxHandler(SplitHandlerId::RGB, &instance);
  }

private:
#if JAVELIN_SPLIT
  struct Ws2812Data final : public SplitTxHandler, SplitRxHandler {
#else
  struct Ws2812Data {
#endif
    bool dirty;
#if JAVELIN_SPLIT
    bool slaveDirty;
#endif

    uint32_t lastUpdate;
    uint32_t pixelValues[JAVELIN_RGB_COUNT];

    void Update();
    void SetRgb(size_t pixelId, uint32_t ws2812Color) {
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
      if (Split::IsLeft()) {
        if (pixelId < JAVELIN_RGB_LEFT_COUNT) {
          dirty = true;
        } else {
          slaveDirty = true;
        }

      } else {
        if (pixelId < JAVELIN_RGB_LEFT_COUNT) {
          slaveDirty = true;
        } else {
          dirty = true;
        }
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

  static Ws2812Data instance;
};

#else

// Do nothing instance.
class Ws2812 {
public:
  static void Initialize() {}
  static void Update() {}
  static bool IsAvailable() { return false; }
  static size_t GetCount() { return 0; }

  static void SetRgb(size_t pixelId, int r, int g, int b) {}
  static void SetRgb(size_t pixelId, uint32_t ws2812Color) {}

  static void RegisterTxHandler() {}
  static void RegisterRxHandler() {}
};

#endif

//---------------------------------------------------------------------------
