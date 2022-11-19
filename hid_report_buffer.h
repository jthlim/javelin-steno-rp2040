//---------------------------------------------------------------------------

#pragma once
#include <stdint.h>
#include <stdlib.h>

//---------------------------------------------------------------------------

struct HidReportBufferEntry {
  uint8_t instance;
  uint8_t reportId;
  uint8_t dataLength;
};

class HidReportBufferBase {
public:
  HidReportBufferBase(uint8_t dataSize, uint8_t *entryData)
      : dataSize(dataSize), entryData(entryData) {}

  void SendReport(uint8_t instance, uint8_t reportId, const uint8_t *data,
                  size_t length);
  void SendNextReport();

  void Print(const char *p);
  static void PrintInfo();

  static uint32_t reportsSentCount[];

protected:
  static const size_t NUMBER_OF_ENTRIES = 16;

private:
  uint8_t startIndex = 0;
  uint8_t endIndex = 0;
  const uint8_t dataSize;
  HidReportBufferEntry entries[NUMBER_OF_ENTRIES] = {};
  uint8_t *entryData;
};

//---------------------------------------------------------------------------

template <size_t DATA_SIZE>
struct HidReportBuffer : public HidReportBufferBase {
public:
  HidReportBuffer() : HidReportBufferBase(DATA_SIZE, buffers) {}

private:
  uint8_t buffers[DATA_SIZE * NUMBER_OF_ENTRIES];
};

//---------------------------------------------------------------------------
