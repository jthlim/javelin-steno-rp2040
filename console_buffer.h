//---------------------------------------------------------------------------

#pragma once
#include "hid_report_buffer.h"
#include <stdint.h>
#include <stdlib.h>

//---------------------------------------------------------------------------

struct ConsoleBuffer {
public:
  ConsoleBuffer() {}

  void SendData(const uint8_t *data, size_t length);
  void Flush();

  static HidReportBuffer reportBuffer;

private:
  static const size_t MAX_BUFFER_SIZE = 63;

  int bufferSize = 0;
  uint8_t buffer[MAX_BUFFER_SIZE];
};

//---------------------------------------------------------------------------
