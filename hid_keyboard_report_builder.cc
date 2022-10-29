//---------------------------------------------------------------------------

#include "hid_keyboard_report_builder.h"
#include "hid_report_buffer.h"
#include "usb_descriptors.h"

#include "tusb.h"
#include <string.h>

//---------------------------------------------------------------------------

void HidKeyboardReportBuilder::Press(uint8_t key) {
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

void HidKeyboardReportBuilder::Release(uint8_t key) {
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

bool HidKeyboardReportBuilder::HasData() const {
  for (size_t i = 0; i < 20; ++i) {
    if (buffers[0].presenceFlags[i] != 0)
      return true;
  }
  return false;
}

void HidKeyboardReportBuilder::Flush() {
  if (HasData()) {
    DoFlush();
    if (HasData()) {
      DoFlush();
    }
  }
}

void HidKeyboardReportBuilder::DoFlush() {
  HidReportBuffer::instance.SendReport(ITF_NUM_KEYBOARD, REPORT_ID_KEYBOARD,
                                       buffers[0].data, 17);

  buffers[0] = buffers[1];
  memset(&buffers[1], 0, sizeof(buffers[1]));
  maxPressIndex = 0;
  if (modifiers != 0) {
    buffers[0].data[0] = modifiers;
    buffers[0].presenceFlags[0] = 1;
  }
}

//---------------------------------------------------------------------------
