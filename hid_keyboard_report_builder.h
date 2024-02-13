//---------------------------------------------------------------------------

#pragma once
#include "hid_report_buffer.h"
#include "javelin/key_code.h"
#include <stddef.h>
#include <stdint.h>

//---------------------------------------------------------------------------

struct HidKeyboardReportBuilder {
public:
  void Press(uint8_t key);
  void Release(uint8_t key);

  void FlushIfRequired();
  void Flush();
  void SendNextReport() { reportBuffer.SendNextReport(); }

  void Reset();
  size_t GetAvailableBufferCount() const {
    return reportBuffer.GetAvailableBufferCount();
  }

  bool IsCompatibilityMode() const { return compatibilityMode; }
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
      uint16_t presenceFlags16[16];
      uint32_t presenceFlags32[8];
    };
  };

  bool compatibilityMode = false;
  uint8_t modifiers = 0;
  uint8_t maxPressIndex = 0;
  Buffer buffers[2];

  static const size_t MAXIMUM_REPORT_DATA_SIZE = 17;
  HidReportBuffer<MAXIMUM_REPORT_DATA_SIZE> reportBuffer;

  bool HasData() const;
  void SendKeyboardPageReportIfRequired();
  void SendConsumerPageReportIfRequired();

  friend class SplitHidReportBuffer;
};

//---------------------------------------------------------------------------
