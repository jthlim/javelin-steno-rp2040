//---------------------------------------------------------------------------

#pragma once
#include "hid_report_buffer.h"
#include "javelin/key_code.h"
#include <stddef.h>
#include <stdint.h>

//---------------------------------------------------------------------------

struct MainReportBuilder {
public:
  void Press(uint8_t key);
  void Release(uint8_t key);

  void PressMouseButton(size_t buttonIndex);
  void ReleaseMouseButton(size_t buttonIndex);
  void MoveMouse(int dx, int dy);
  void WheelMouse(int delta);

  void FlushAllIfRequired();
  void Flush();
  void FlushMouse();
  void SendNextReport() { reportBuffer.SendNextReport(); }

  void Reset();
  size_t GetAvailableBufferCount() const {
    return reportBuffer.GetAvailableBufferCount();
  }

  bool IsCompatibilityMode() const { return compatibilityMode; }
  void SetCompatibilityMode(bool mode) { compatibilityMode = mode; }

  void PrintInfo() const;

  static MainReportBuilder instance;

private:
  MainReportBuilder();

  struct Buffer {
    union {
      uint8_t data[32];
      uint32_t data32[8];
    };

    union {
      uint8_t presenceFlags[32];
      uint16_t presenceFlags16[16];
      uint32_t presenceFlags32[8];
    };
  };

  struct MouseBuffer {
    // Order is important. First 7 bytes is the report packet.
    uint32_t buttonData;
    int8_t dx, dy, wheel;
    uint8_t movementMask; // Bit 0 = dx,dy, Bit 1 = wheel
    uint32_t buttonPresence;

    void Reset() {
      movementMask = 0;
      dx = 0;
      dy = 0;
      buttonData = 0;
      buttonPresence = 0;
    }

    void SetMove(int x, int y) {
      dx = x;
      dy = y;
      movementMask |= 1;
    }

    void SetWheel(int delta) {
      wheel = delta;
      movementMask |= 2;
    }

    bool HasMovement() const { return (movementMask & 1) != 0; }
    bool HasWheel() const { return (movementMask & 2) != 0; }

    bool HasData() const { return (movementMask | buttonPresence) != 0; }
  };

  bool compatibilityMode = false;
  uint8_t modifiers = 0;
  uint8_t maxPressIndex = 0;
  Buffer buffers[2];
  MouseBuffer mouseBuffers[2];

  static const size_t MAXIMUM_REPORT_DATA_SIZE = 17;
  HidReportBuffer<MAXIMUM_REPORT_DATA_SIZE> reportBuffer;

  bool HasData() const;
  bool HasMouseData() const { return mouseBuffers[0].HasData(); }
  void SendKeyboardPageReportIfRequired();
  void SendConsumerPageReportIfRequired();
  void SendMousePageReportIfRequired();

  friend class SplitHidReportBuffer;
};

//---------------------------------------------------------------------------
