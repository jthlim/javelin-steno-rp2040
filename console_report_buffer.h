//---------------------------------------------------------------------------

#pragma once
#include "hid_report_buffer.h"

//---------------------------------------------------------------------------

struct ConsoleReportBuffer {
public:
  void SendData(const uint8_t *data, size_t length);
  void Flush();

  void SendNextReport() { reportBuffer.SendNextReport(); }
  void Reset() {
    bufferSize = 0;
    reportBuffer.Reset();
  }
  size_t GetAvailableBufferCount() const {
    return reportBuffer.GetAvailableBufferCount();
  }

  static ConsoleReportBuffer instance;

private:
  ConsoleReportBuffer();

  static const size_t MAX_BUFFER_SIZE = 64;

  int bufferSize = 0;
  uint8_t buffer[MAX_BUFFER_SIZE];

  HidReportBuffer<64> reportBuffer;

  friend class SplitHidReportBuffer;
};

//---------------------------------------------------------------------------
