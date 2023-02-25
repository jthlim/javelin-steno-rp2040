//---------------------------------------------------------------------------

#pragma once
#include "javelin/stroke.h"
#include "split_tx_rx.h"

//---------------------------------------------------------------------------

#if JAVELIN_OLED_DRIVER == 1306

class Ssd1306 {
public:
  static void Initialize() { GetInstance().Initialize(); }
  static void PrintInfo();

  static void Update() { GetInstance().Update(); }

  static void DrawPaperTape(int displayId, const StenoStroke *strokes,
                            size_t length) {
    instances[displayId].DrawPaperTape(strokes, length);
  }
  static void DrawStenoLayout(int displayId, StenoStroke stroke) {
    instances[displayId].DrawStenoLayout(stroke);
  }

#if JAVELIN_SPLIT
  static void RegisterMasterHandlers() {
    SplitTxRx::RegisterRxHandler(SplitHandlerId::OLED_AVAILABLE,
                                 &instances[1].available);
    SplitTxRx::RegisterTxHandler(&instances[1]);
  }
  static void RegisterSlaveHandlers() {
    SplitTxRx::RegisterTxHandler(&instances[1].available);
    SplitTxRx::RegisterRxHandler(SplitHandlerId::OLED_DATA, &instances[1]);
  }
#else
  static void RegisterMasterHandlers() {}
  static void RegisterSlaveHandlers() {}
#endif

private:
  class Ssd1306Availability : public SplitTxHandler, public SplitRxHandler {
  public:
    void operator=(bool value) { available = value; }
    bool operator!() const { return !available; }
    operator bool() const { return available; }

  private:
    bool available = true; // During startup scripts, the frame buffer may
                           // need to be written.
                           // Setting this to available will permit
                           // the slave OLED to be updated until the first
                           // availability packet comes in.
    bool dirty = true;

    virtual void UpdateBuffer(TxBuffer &buffer);
    virtual void OnDataReceived(const void *data, size_t length);
    virtual void OnReceiveConnectionReset() { available = false; }
    virtual void OnTransmitConnectionReset() { dirty = true; }
  };

  class Ssd1306Data : public SplitTxHandler, public SplitRxHandler {
  public:
    Ssd1306Availability available;
    bool dirty;

    void Initialize();

    // None of these will take effect until Update() is called.
    void Clear();
    void DrawLine(int x0, int y0, int x1, int y1, bool on);
    void DrawImage(int x, int y, int width, int height, const uint8_t *data);
    void DrawText(int x, int y, int fontId, const uint8_t *text);
    void SetPixel(uint32_t x, uint32_t y, bool on);

    void DrawPaperTape(const StenoStroke *strokes, size_t length);
    void DrawStenoLayout(StenoStroke stroke);

    void Update();
    void PrintInfo();

  private:
    union {
      uint8_t buffer8[JAVELIN_OLED_WIDTH * JAVELIN_OLED_HEIGHT / 8];
      uint32_t buffer32[JAVELIN_OLED_WIDTH * JAVELIN_OLED_HEIGHT / 32];
    };

    static uint16_t dmaBuffer[JAVELIN_OLED_WIDTH * JAVELIN_OLED_HEIGHT / 8 + 1];

    bool IsI2cTxReady() const;
    void WaitForI2cTxReady() const;

    bool InitializeSsd1306();
    bool SendCommandList(const uint8_t *commands, size_t length);
    bool SendCommand(uint8_t command);
    void SendData(const void *data, size_t length);

    virtual void UpdateBuffer(TxBuffer &buffer);
    virtual void OnTransmitConnectionReset() { dirty = true; }
    virtual void OnDataReceived(const void *data, size_t length);
  };

#if JAVELIN_SPLIT
  static Ssd1306Data &GetInstance() {
    if (SplitTxRx::IsMaster()) {
      return instances[0];
    } else {
      return instances[1];
    }
  }
  static Ssd1306Data instances[2];
#else
  static Ssd1306Data &GetInstance() { return instance[0]; }
  static Ssd1306Data instances[1];
#endif

  friend class Display;
};

#else

class Ssd1306 {
public:
  static void Initialize() {}
  static void PrintInfo() {}
  static void DrawPaperTape() {}

  static void Update() {}

  static void RegisterMasterHandlers() {}
  static void RegisterSlaveHandlers() {}
};

#endif

//---------------------------------------------------------------------------
