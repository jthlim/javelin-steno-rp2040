//---------------------------------------------------------------------------

#pragma once
#include <stddef.h>
#include <stdint.h>

//---------------------------------------------------------------------------

class HidReportBufferBase {
public:
  void SendReport(uint8_t reportId, const uint8_t *data, size_t length);
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
  HidReportBufferBase(uint8_t entrySize, uint8_t instanceNumber)
      : entrySize(entrySize), instanceNumber(instanceNumber) {}

  static const size_t NUMBER_OF_ENTRIES = 16;

private:
  const uint8_t entrySize;
  const uint8_t instanceNumber;
  size_t startIndex = 0;
  size_t endIndex = 0;

  struct Entry {
    uint8_t length;
    uint8_t reportId;
    uint8_t data[1];
  };

  Entry *GetEntry(size_t index) {
    return (Entry *)(entryData + entrySize * index);
  }

public:
  uint8_t entryData[0];
};

//---------------------------------------------------------------------------

template <size_t DATA_SIZE>
struct HidReportBuffer : public HidReportBufferBase {
public:
  HidReportBuffer(uint8_t instanceNumber)
      : HidReportBufferBase(DATA_SIZE + 2, instanceNumber) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
    static_assert(offsetof(HidReportBuffer, buffers) ==
                      offsetof(HidReportBufferBase, entryData),
                  "Data is not positioned where expected");
#pragma GCC diagnostic pop
  }

private:
  // Each entry has one byte prefix which is the length, followed by the data.
  uint8_t buffers[(DATA_SIZE + 2) * NUMBER_OF_ENTRIES];
};

//---------------------------------------------------------------------------
