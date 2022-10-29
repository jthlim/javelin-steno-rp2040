//---------------------------------------------------------------------------

#include "hid_report_buffer.h"
#include "usb_descriptors.h"

#include "tusb.h"
#include <string.h>

//---------------------------------------------------------------------------

HidReportBuffer HidReportBuffer::instance;

//---------------------------------------------------------------------------

void HidReportBuffer::Print(const char *p) {
  while (*p) {
    uint8_t buffer[25] = {0};
    int c = *p++;
    int value = 0;
    if ('A' <= c && c <= 'Z') {
      buffer[0] = 2; // Shift
      value = c - 'A' + HID_KEY_A;
    } else if ('a' <= c && c <= 'z') {
      value = c - 'a' + HID_KEY_A;
    } else if ('1' <= c && c <= '9') {
      value = c - '1' + HID_KEY_1;
    } else if (c == '0') {
      value = HID_KEY_0;
    } else if (c == ' ') {
      value = HID_KEY_SPACE;
    } else if (c == '-') {
      value = HID_KEY_MINUS;
    } else if (c == '*') {
      buffer[0] = 2;
      value = HID_KEY_8;
    }
    if (value) {
      buffer[value / 8 + 1] |= (1 << (value % 8));
      SendReport(REPORT_ID_KEYBOARD, buffer, sizeof(buffer));
      uint8_t empty[25] = {0};
      SendReport(REPORT_ID_KEYBOARD, empty, sizeof(empty));
    }
  }
}

//---------------------------------------------------------------------------

void HidReportBuffer::SendReport(uint8_t reportId, const uint8_t *data,
                                 size_t length) {
  while (endIndex >= startIndex + 8) {
    // Wait!
    tud_task();
  }

  bool triggerSend = startIndex == endIndex;
  if (triggerSend) {
    ++endIndex;
    tud_hid_report(reportId, data, length);
  } else {
    HidReportBufferEntry *entry = &entries[endIndex & 7];
    ++endIndex;
    entry->reportId = reportId;
    entry->dataLength = length;
    memcpy(entry->data, data, length);
  }
}

//---------------------------------------------------------------------------

void HidReportBuffer::SendNextReport() {

  // This should never happen!
  if (startIndex == endIndex)
    return;

  ++startIndex;
  if (startIndex >= 8) {
    startIndex -= 8;
    endIndex -= 8;
  }

  if (startIndex == endIndex) {
    return;
  }

  HidReportBufferEntry *entry = &entries[startIndex & 7];

  tud_hid_report(entry->reportId, entry->data, entry->dataLength);
}

//---------------------------------------------------------------------------

void HidReportBuilder::Press(uint8_t key) {
  if (key == 0) {
    return;
  }
  if (0xe0 <= key && key < 0xe8) {
    modifiers |= (1 << (key - 0xe0));
    if (maxPressIndex == 0) {
      buffers[0].data[0] = modifiers;
      buffers[0].presenceFlags[0] = 1;
    }
    return;
  }
  if (key >= 128) {
    return;
  }

  int byte = 1 + (key >> 3);
  int mask = (1 << (key & 7));

  if (key <= maxPressIndex ||
      (maxPressIndex != 0 && buffers[0].data[0] != modifiers) ||
      (buffers[0].presenceFlags[byte] & mask)) {
    DoFlush();
  }

  if (buffers[0].presenceFlags[byte] & mask) {
    DoFlush();
  }

  buffers[0].data[0] = modifiers;
  buffers[0].presenceFlags[0] = 1;
  buffers[0].data[byte] |= mask;
  buffers[0].presenceFlags[byte] |= mask;
  if (key > maxPressIndex) {
    maxPressIndex = key;
  }
}

void HidReportBuilder::Release(uint8_t key) {
  if (key == 0) {
    return;
  }
  if (0xe0 <= key && key < 0xe8) {
    modifiers &= ~(1 << (key - 0xe0));
    if (maxPressIndex == 0) {
      buffers[0].data[0] = modifiers;
      buffers[0].presenceFlags[0] = 1;
    }
    return;
  }

  if (key >= 128) {
    return;
  }

  int byte = 1 + (key >> 3);
  int mask = (1 << (key & 7));

  if (buffers[0].data[byte] & mask) {
    buffers[1].presenceFlags[byte] |= mask;
  } else {
    buffers[0].presenceFlags[byte] |= mask;
  }
}


bool HidReportBuilder::HasData() const {
  for (size_t i = 0; i < 20; ++i) {
    if (buffers[0].presenceFlags[i] != 0)
      return true;
  }
  return false;
}

void HidReportBuilder::Flush() {
  if (HasData()) {
    DoFlush();
  }
}

void HidReportBuilder::DoFlush() {
  HidReportBuffer::instance.SendReport(REPORT_ID_KEYBOARD, buffers[0].data, 17);

  buffers[0] = buffers[1];
  memset(&buffers[1], 0, sizeof(buffers[1]));
  maxPressIndex = 0;
  if (modifiers != 0) {
    buffers[0].data[0] = modifiers;
    buffers[0].presenceFlags[0] = 1;
  }
}

//---------------------------------------------------------------------------
