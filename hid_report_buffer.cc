//---------------------------------------------------------------------------

#include "hid_report_buffer.h"
#include "usb_descriptors.h"

#include "tusb.h"
#include <string.h>

//---------------------------------------------------------------------------

void HidReportBuffer::SendReport(uint8_t instance, uint8_t reportId,
                                 const uint8_t *data, size_t length) {
  while (endIndex >= startIndex + NUMBER_OF_ENTRIES) {
    // Wait!
    tud_task();
  }

  bool triggerSend = startIndex == endIndex;
  if (triggerSend) {
    ++endIndex;
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

  tud_hid_n_report(entry->instance, entry->reportId, entry->data,
                   entry->dataLength);
}

//---------------------------------------------------------------------------
