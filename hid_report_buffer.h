//---------------------------------------------------------------------------

#pragma once
#include <stddef.h>
#include <stdint.h>

//---------------------------------------------------------------------------

class HidReportBufferBase {
public:
  void SendReport(const uint8_t *data, size_t length);
  void SendNextReport();

  void Print(const char *p);
  void Reset() {
    startIndex = 0;
    endIndex = 0;
  }

  bool IsEmpty() const { return startIndex == endIndex; }
  bool IsFull() const { return endIndex - startIndex >= NUMBER_OF_ENTRIES; }
  size_t GetAvailableBufferCount() const {
    size_t used = endIndex - startIndex;
    return NUMBER_OF_ENTRIES - used;
  }

  static void PrintInfo();

  static uint32_t reportsSentCount[];

protected:
  HidReportBufferBase(uint8_t entrySize, uint8_t instanceNumber,
                      uint8_t reportId)
      : entrySize(entrySize), instanceNumber(instanceNumber),
        reportId(reportId) {}

  static const size_t NUMBER_OF_ENTRIES = 16;

private:
  size_t startIndex = 0;
  size_t endIndex = 0;
  const uint8_t entrySize;
  const uint8_t instanceNumber;
  const uint8_t reportId;

public:
  uint8_t entryData[0];
};

//---------------------------------------------------------------------------

template <size_t DATA_SIZE>
struct HidReportBuffer : public HidReportBufferBase {
public:
  HidReportBuffer(uint8_t instanceNumber, uint8_t reportId)
      : HidReportBufferBase(DATA_SIZE + 1, instanceNumber, reportId) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
    static_assert(offsetof(HidReportBuffer, buffers) ==
                      offsetof(HidReportBufferBase, entryData),
                  "Data is not positioned where expected");
#pragma GCC diagnostic pop
  }

private:
  // Each entry has one byte prefix which is the length, followed by the data.
  uint8_t buffers[(DATA_SIZE + 1) * NUMBER_OF_ENTRIES];
};

//---------------------------------------------------------------------------
