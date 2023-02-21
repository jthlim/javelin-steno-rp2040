//---------------------------------------------------------------------------

#pragma once
#include "hid_report_buffer.h"
#include <stdint.h>
#include <stdlib.h>

//---------------------------------------------------------------------------

struct ConsoleBuffer {
public:
  void SendData(const uint8_t *data, size_t length);
  void Flush();

  void SendNextReport() { reportBuffer.SendNextReport(); }
  void Reset() { reportBuffer.Reset(); }

  static ConsoleBuffer instance;

private:
  ConsoleBuffer() = default;

  static const size_t MAX_BUFFER_SIZE = 64;

  int bufferSize = 0;
  uint8_t buffer[MAX_BUFFER_SIZE];

  HidReportBuffer<64> reportBuffer;
};

//---------------------------------------------------------------------------
