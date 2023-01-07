//---------------------------------------------------------------------------

#pragma once
#include "hid_report_buffer.h"
#include <stdint.h>
#include <stdlib.h>

//---------------------------------------------------------------------------

struct HidKeyboardReportBuilder {
public:
  void Press(uint8_t key);
  void Release(uint8_t key);

  bool HasData() const;
  void FlushIfRequired();
  void Flush();
  void SendNextReport() { reportBuffer.SendNextReport(); }

  void SetCompatibilityMode(bool mode) { compatibilityMode = mode; }

  static HidKeyboardReportBuilder instance;

private:
  HidKeyboardReportBuilder() = default;

  struct Buffer {
    uint8_t data[32];
    uint8_t presenceFlags[32];
  };

  bool compatibilityMode = false;
  uint8_t modifiers = 0;
  uint8_t maxPressIndex = 0;
  Buffer buffers[2] = {};

  HidReportBuffer<32> reportBuffer;
};

//---------------------------------------------------------------------------
