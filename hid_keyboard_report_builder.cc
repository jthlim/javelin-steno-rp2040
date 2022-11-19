//---------------------------------------------------------------------------

#include "hid_keyboard_report_builder.h"
#include "hid_report_buffer.h"
#include "usb_descriptors.h"

#include <string.h>
#include <tusb.h>

//---------------------------------------------------------------------------

HidReportBuffer<32> HidKeyboardReportBuilder::reportBuffer;
bool HidKeyboardReportBuilder::compatibilityMode = false;

//---------------------------------------------------------------------------

const size_t MODIFIER_OFFSET = 0;

void HidKeyboardReportBuilder::Press(uint8_t key) {
  if (key == 0) {
    return;
  }
  if (0xe0 <= key && key < 0xe8) {
    modifiers |= (1 << (key - 0xe0));
    if (maxPressIndex == 0) {
      buffers[0].data[MODIFIER_OFFSET] = modifiers;
      buffers[0].presenceFlags[MODIFIER_OFFSET] = 0xff;
    } else {
      buffers[1].data[MODIFIER_OFFSET] = modifiers;
      buffers[1].presenceFlags[MODIFIER_OFFSET] = 0xff;
    }
    if (compatibilityMode) {
      Flush();
    }
    return;
  }

  int byte = (key >> 3);
  if (key < 0xe0) {
    ++byte;
  }
  int mask = (1 << (key & 7));

  if (key <= maxPressIndex ||
      (maxPressIndex != 0 && buffers[0].data[MODIFIER_OFFSET] != modifiers) ||
      (buffers[0].presenceFlags[byte] & mask)) {
    Flush();
  }

  if (buffers[0].presenceFlags[byte] & mask) {
    Flush();
  }

  buffers[0].data[MODIFIER_OFFSET] = modifiers;
  buffers[0].presenceFlags[MODIFIER_OFFSET] = 1;
  buffers[0].data[byte] |= mask;
  buffers[0].presenceFlags[byte] |= mask;
  if (key > maxPressIndex) {
    maxPressIndex = key;
  }

  if (compatibilityMode) {
    Flush();
  }
}

void HidKeyboardReportBuilder::Release(uint8_t key) {
  if (key == 0) {
    return;
  }
  if (0xe0 <= key && key < 0xe8) {
    modifiers &= ~(1 << (key - 0xe0));
    if (maxPressIndex == 0) {
      buffers[0].data[MODIFIER_OFFSET] = modifiers;
      buffers[0].presenceFlags[MODIFIER_OFFSET] = 0xff;
    } else {
      buffers[1].data[MODIFIER_OFFSET] = modifiers;
      buffers[1].presenceFlags[MODIFIER_OFFSET] = 0xff;
    }
    if (compatibilityMode) {
      Flush();
    }
    return;
  }

  int byte = (key >> 3);
  if (key < 0xe0) {
    ++byte;
  }
  int mask = (1 << (key & 7));

  if ((buffers[0].presenceFlags[byte] & mask) != 0 &&
      (buffers[0].data[byte] & mask) != 0) {
    buffers[1].presenceFlags[byte] |= mask;
    buffers[1].data[byte] &= ~mask;
  } else {
    buffers[0].presenceFlags[byte] |= mask;
    buffers[0].data[byte] &= ~mask;
  }
  if (compatibilityMode) {
    Flush();
  }
}

bool HidKeyboardReportBuilder::HasData() const {
  for (size_t i = 0; i < 32; ++i) {
    if (buffers[0].presenceFlags[i] != 0)
      return true;
  }
  return false;
}

void HidKeyboardReportBuilder::FlushIfRequired() {
  if (HasData()) {
    Flush();
    if (HasData()) {
      Flush();
    }
  }
}

void HidKeyboardReportBuilder::Flush() {
  reportBuffer.SendReport(ITF_NUM_KEYBOARD, 0, buffers[0].data, 32);

  for (int i = 0; i < 32; ++i) {
    buffers[0].data[i] = (buffers[1].presenceFlags[i] & buffers[1].data[i]) |
                         (~buffers[1].presenceFlags[i] & buffers[0].data[i]);

    buffers[0].presenceFlags[i] = buffers[1].presenceFlags[i];
  }

  memset(&buffers[1], 0, sizeof(buffers[1]));
  maxPressIndex = 0;
}

//---------------------------------------------------------------------------
