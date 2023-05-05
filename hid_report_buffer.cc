//---------------------------------------------------------------------------

#include "hid_report_buffer.h"
#include "javelin/console.h"
#include "split_hid_report_buffer.h"
#include "usb_descriptors.h"

#include <string.h>
#include <tusb.h>

//---------------------------------------------------------------------------

uint32_t HidReportBufferBase::reportsSentCount[ITF_NUM_TOTAL] = {};

//---------------------------------------------------------------------------

void HidReportBufferBase::SendReport(const uint8_t *data, size_t length) {
  assert(length == dataSize);
  do {
    tud_task();
  } while (IsFull());

  bool triggerSend = startIndex == endIndex;
  if (triggerSend) {
    if (!tud_hid_n_ready(instanceNumber)) {
#if JAVELIN_SPLIT
      if (Split::IsMaster()) {
        reportsSentCount[instanceNumber]++;
        SplitHidReportBuffer::Add(instanceNumber, reportId, data, length);
      }
#endif
      return;
    }

    if (tud_hid_n_report(instanceNumber, reportId, data, dataSize)) {
      ++endIndex;
      reportsSentCount[instanceNumber]++;
    }
    return;
  }

  uint8_t *buffer = entryData + dataSize * (endIndex & (NUMBER_OF_ENTRIES - 1));
  ++endIndex;
  memcpy(buffer, data, dataSize);
}

//---------------------------------------------------------------------------

void HidReportBufferBase::SendNextReport() {
  // This should never happen!
  if (startIndex == endIndex) {
    return;
  }

  for (;;) {
    ++startIndex;
    if (startIndex == endIndex) {
      return;
    }

    uint8_t *buffer =
        entryData + dataSize * (startIndex & (NUMBER_OF_ENTRIES - 1));

    reportsSentCount[instanceNumber]++;
    if (tud_hid_n_report(instanceNumber, reportId, buffer, dataSize)) {
      return;
    }
  }
}

//---------------------------------------------------------------------------

void HidReportBufferBase::PrintInfo() {
  Console::Printf("HID reports sent\n");
  Console::Printf("  Keyboard: %u\n", reportsSentCount[ITF_NUM_KEYBOARD]);
  Console::Printf("  Plover HID: %u\n", reportsSentCount[ITF_NUM_PLOVER_HID]);
  Console::Printf("  Console: %u\n", reportsSentCount[ITF_NUM_CONSOLE]);
}

//---------------------------------------------------------------------------
