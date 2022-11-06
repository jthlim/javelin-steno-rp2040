//---------------------------------------------------------------------------

#pragma once
#include <stdint.h>
#include <stdlib.h>

//---------------------------------------------------------------------------

struct HidReportBufferEntry {
  uint8_t instance;
  uint8_t reportId;
  uint8_t dataLength;
  uint8_t data[64];
};

struct HidReportBuffer {
public:
  HidReportBuffer() {}

  void SendReport(uint8_t instance, uint8_t reportId, const uint8_t *data,
                  size_t length);
  void SendNextReport();

  void Print(const char *p);
  static void PrintInfo();

  static uint32_t reportsSentCount[];

private:
  static const size_t NUMBER_OF_ENTRIES = 16;

  uint8_t startIndex = 0;
  uint8_t endIndex = 0;
  HidReportBufferEntry entries[NUMBER_OF_ENTRIES] = {};
};

//---------------------------------------------------------------------------
