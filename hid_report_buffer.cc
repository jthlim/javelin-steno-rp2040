//---------------------------------------------------------------------------

#include "hid_report_buffer.h"
#include "javelin/console.h"
#include "usb_descriptors.h"

#include <string.h>
#include <tusb.h>

//---------------------------------------------------------------------------

uint32_t HidReportBuffer::reportsSentCount[ITF_NUM_TOTAL] = {};

//---------------------------------------------------------------------------

void HidReportBuffer::SendReport(uint8_t instance, uint8_t reportId,
                                 const uint8_t *data, size_t length) {
  do {
    tud_task();
  } while (endIndex >= startIndex + NUMBER_OF_ENTRIES);

  bool triggerSend = startIndex == endIndex;
  if (triggerSend) {
    ++endIndex;
    reportsSentCount[instance]++;
    tud_hid_n_report(instance, reportId, data, length);
    return;
  }

  HidReportBufferEntry *entry = &entries[endIndex & (NUMBER_OF_ENTRIES - 1)];
  ++endIndex;
  entry->instance = instance;
  entry->reportId = reportId;
  entry->dataLength = length;
  memcpy(entry->data, data, length);
}

//---------------------------------------------------------------------------

void HidReportBuffer::SendNextReport() {

  // This should never happen!
  if (startIndex == endIndex)
    return;

  ++startIndex;
  if (startIndex >= NUMBER_OF_ENTRIES) {
    startIndex -= NUMBER_OF_ENTRIES;
    endIndex -= NUMBER_OF_ENTRIES;
  }

  if (startIndex == endIndex) {
    return;
  }

  HidReportBufferEntry *entry = &entries[startIndex & (NUMBER_OF_ENTRIES - 1)];

  reportsSentCount[entry->instance]++;
  tud_hid_n_report(entry->instance, entry->reportId, entry->data,
                   entry->dataLength);
}

//---------------------------------------------------------------------------

void HidReportBuffer::PrintInfo() {
  Console::Printf("HID reports sent\n");
  Console::Printf("  Keyboard: %u\n", reportsSentCount[ITF_NUM_KEYBOARD]);
  Console::Printf("  Plover HID: %u\n", reportsSentCount[ITF_NUM_PLOVER_HID]);
  Console::Printf("  Console: %u\n", reportsSentCount[ITF_NUM_CONSOLE]);
}

//---------------------------------------------------------------------------
