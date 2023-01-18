//---------------------------------------------------------------------------

#include "config/uni_v4.h"
#include "console_buffer.h"
#include "hid_keyboard_report_builder.h"
#include "javelin/debounce.h"
#include "javelin/key.h"
#include "javelin/static_allocate.h"
#include "key_state.h"
#include "plover_hid_report_buffer.h"
#include "rp2040_crc32.h"
#include "usb_descriptors.h"

#include <stdlib.h>
#include <string.h>
#include <tusb.h>

//---------------------------------------------------------------------------

void OnConsoleReceiveData(const uint8_t *data, uint8_t length);
void InitJavelinSteno();
void ProcessStenoTick();
void InitMulticore();

//---------------------------------------------------------------------------

const uint64_t DEBOUNCE_TIME_US = 5000;
int resumeCount = 0;

//---------------------------------------------------------------------------
// Device callbacks
//---------------------------------------------------------------------------

// Invoked when device is mounted
void tud_mount_cb(void) {}

// Invoked when device is unmounted
void tud_umount_cb(void) {}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
  // set_sys_clock_khz(25000, true);
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
  // set_sys_clock_khz(125000, true);
  ++resumeCount;
}

//---------------------------------------------------------------------------
// USB HID
//---------------------------------------------------------------------------

class HidTask {
public:
  HidTask() : buttonManager(BUTTON_MANAGER_BYTE_CODE) {}

  void Update();

private:
  bool isReady = false;
  GlobalDeferredDebounce<ButtonState> debouncer;
  ButtonManager buttonManager;
};

void HidTask::Update() {
  Debounced<ButtonState> keyState = debouncer.Update(KeyState::Read());
  if (!keyState.isUpdated) {
    return;
  }

  if (tud_suspended()) {
    if (keyState.value.IsAnySet()) {
      // Wake up host if we are in suspend mode
      // and REMOTE_WAKEUP feature is enabled by host
      tud_remote_wakeup();
    }
  } else if (isReady) {
    buttonManager.Update(keyState.value);
  } else if (tud_hid_n_ready(ITF_NUM_KEYBOARD)) {
    isReady = true;
    buttonManager.Update(keyState.value);
  }
}

// Invoked when received SET_PROTOCOL request
// protocol is either HID_PROTOCOL_BOOT (0) or HID_PROTOCOL_REPORT (1)
void tud_hid_set_protocol_cb(uint8_t instance, uint8_t protocol) {}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, const uint8_t *report,
                                uint8_t len) {
  switch (instance) {
  case ITF_NUM_KEYBOARD:
    HidKeyboardReportBuilder::instance.SendNextReport();
    break;
  case ITF_NUM_PLOVER_HID:
    PloverHidReportBuffer::instance.SendNextReport();
    break;
  case ITF_NUM_CONSOLE:
    ConsoleBuffer::instance.SendNextReport();
    break;
  }
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, const uint8_t *buffer,
                           uint16_t bufsize) {
  switch (instance) {
  case ITF_NUM_KEYBOARD:
    if (report_type != HID_REPORT_TYPE_OUTPUT || bufsize < 1) {
      return;
    }
    Key::SetIsNumLockOn((buffer[0] & KEYBOARD_LED_NUMLOCK) != 0);
    break;

  case ITF_NUM_CONSOLE:
    OnConsoleReceiveData(buffer, bufsize);
    break;
  }
}

//---------------------------------------------------------------------------

static void cdc_task() {
  uint8_t itf;

  for (itf = 0; itf < CFG_TUD_CDC; itf++) {
    // connected() check for DTR bit
    // Most but not all terminal client set this when making connection
    // if ( tud_cdc_n_connected(itf) )
    {
      if (tud_cdc_n_available(itf)) {
        uint8_t buf[64];
        uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));
        // Do nothing with it.
      }
    }
  }
}

//---------------------------------------------------------------------------

JavelinStaticAllocate<HidTask> hidTaskContainer;

int main(void) {
#if JAVELIN_THREADS
  InitMulticore();
#endif
  KeyState::Init();
  Rp2040Crc32::Initialize();
  InitJavelinSteno();
  new (hidTaskContainer) HidTask;

  tusb_init();

  while (1) {
    tud_task(); // tinyusb device task
    hidTaskContainer->Update();
    cdc_task();

    ProcessStenoTick();
    sleep_us(100);
  }

  return 0;
}

//---------------------------------------------------------------------------
