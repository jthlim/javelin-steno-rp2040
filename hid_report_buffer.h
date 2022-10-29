//---------------------------------------------------------------------------

#pragma once
#include <stdint.h>
#include <stdlib.h>

//---------------------------------------------------------------------------

struct HidReportBufferEntry {
  uint8_t reportId;
  uint8_t dataLength;
  uint8_t data[20];

  void Send() const;
};

struct HidReportBuffer {
public:
  HidReportBuffer() {}

  void SendReport(uint8_t reportId, const uint8_t *data, size_t length);
  void SendNextReport();

  void Print(const char *p);

  static HidReportBuffer instance;

private:
  static const size_t NUMBER_OF_ENTRIES = 16;

  uint8_t startIndex = 0;
  uint8_t endIndex = 0;
  HidReportBufferEntry entries[NUMBER_OF_ENTRIES] = {};
};

//---------------------------------------------------------------------------

struct HidReportBuilder {
public:
  void Press(uint8_t key);
  void Release(uint8_t key);

  bool HasData() const;
  void Flush();

private:
  struct Buffer {
    uint8_t data[20];
    uint8_t presenceFlags[20];
  };

  uint8_t modifiers = 0;
  uint8_t maxPressIndex = 0;
  Buffer buffers[2] = {};

  void DoFlush();
};

//---------------------------------------------------------------------------

