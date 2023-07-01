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
  do {
    tud_task();
  } while (IsFull());

  bool triggerSend = startIndex == endIndex;
  if (triggerSend) {
    if (!tud_hid_n_ready(instanceNumber)) {
#if JAVELIN_SPLIT
      if (Split::IsMaster() &&
          Connection::IsPairConnected(PairConnectionId::ACTIVE)) {
        reportsSentCount[instanceNumber]++;
        SplitHidReportBuffer::Add(instanceNumber, reportId, data, length);
      }
#endif
      return;
    }

    if (tud_hid_n_report(instanceNumber, reportId, data, length)) {
      ++endIndex;
      reportsSentCount[instanceNumber]++;
    }
    return;
  }

  uint8_t *buffer =
      entryData + entrySize * (endIndex & (NUMBER_OF_ENTRIES - 1));
  ++endIndex;
  *buffer = length;
  memcpy(buffer + 1, data, length);
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

    const uint8_t *buffer =
        entryData + entrySize * (startIndex & (NUMBER_OF_ENTRIES - 1));
    size_t length = *buffer;
    const uint8_t *data = buffer + 1;

    reportsSentCount[instanceNumber]++;
    if (tud_hid_n_report(instanceNumber, reportId, data, length)) {
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
