//---------------------------------------------------------------------------

#pragma once
#include "hid_report_buffer.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

//---------------------------------------------------------------------------

struct HidKeyboardReportBuilder {
public:
  void Press(uint8_t key);
  void Release(uint8_t key);

  void FlushIfRequired();
  void Flush();
  void SendNextReport() { reportBuffer.SendNextReport(); }

  void SetCompatibilityMode(bool mode) { compatibilityMode = mode; }

  void PrintInfo() const;

  static HidKeyboardReportBuilder instance;

private:
  HidKeyboardReportBuilder();

  struct Buffer {
    union {
      uint8_t data[32];
      uint32_t data32[8];
    };

    union {
      uint8_t presenceFlags[32];
      uint32_t presenceFlags32[8];
    };
  };

  bool compatibilityMode = false;
  uint8_t modifiers = 0;
  uint8_t maxPressIndex = 0;
  Buffer buffers[2];

  HidReportBuffer<32> reportBuffer;

  bool HasData() const;
};

//---------------------------------------------------------------------------
