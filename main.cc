//---------------------------------------------------------------------------

#include "console_buffer.h"
#include "hid_report_buffer.h"
#include "key_state.h"
#include "usb_descriptors.h"

#include "tusb.h"

#include <hardware/timer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//---------------------------------------------------------------------------

static void hid_task(void);
static void cdc_task(void);

void OnConsoleReceiveData(const uint8_t *data, uint8_t length);
void InitJavelinSteno();
void ProcessStenoKeyState(StenoKeyState keyState);
void ProcessStenoTick();
void SetIsNumLockOn(bool value);

//---------------------------------------------------------------------------

const uint64_t DEBOUNCE_TIME_US = 5000;

//---------------------------------------------------------------------------

int main(void) {
  KeyState::Init();
  InitJavelinSteno();
  tusb_init();

  while (1) {
    tud_task(); // tinyusb device task
    hid_task();
    cdc_task();

    ProcessStenoTick();
    // sleep_ms(1);
  }

  return 0;
}

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
void tud_suspend_cb(bool remote_wakeup_en) { (void)remote_wakeup_en; }

// Invoked when usb bus is resumed
void tud_resume_cb(void) {}

//---------------------------------------------------------------------------
// USB HID
//---------------------------------------------------------------------------

void hid_task(void) {
  static StenoKeyState lastKeyState(0);
  static uint64_t lastTriggerTime = 0;

  StenoKeyState keyState = KeyState::Read();
  if (keyState == lastKeyState) {
    return;
  }

  if (tud_suspended() && keyState.IsNotEmpty()) {
    // Wake up host if we are in suspend mode
    // and REMOTE_WAKEUP feature is enabled by host
    tud_remote_wakeup();
  } else {
    // keyboard interface
    if (tud_hid_n_ready(ITF_NUM_KEYBOARD)) {
      uint64_t now = time_us_64();
      if (now - lastTriggerTime > DEBOUNCE_TIME_US) {
        lastTriggerTime = now;
        ProcessStenoKeyState(keyState);
        lastKeyState = keyState;
      }
    }
  }
}

// Invoked when received SET_PROTOCOL request
// protocol is either HID_PROTOCOL_BOOT (0) or HID_PROTOCOL_REPORT (1)
void tud_hid_set_protocol_cb(uint8_t instance, uint8_t protocol) {
  (void)instance;
  (void)protocol;
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, const uint8_t *report,
                                uint8_t len) {
  switch (instance) {
  case ITF_NUM_KEYBOARD:
    HidReportBuffer::instance.SendNextReport();
    break;
  case ITF_NUM_CONSOLE:
    ConsoleBuffer::reportBuffer.SendNextReport();
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
    SetIsNumLockOn((buffer[0] & KEYBOARD_LED_NUMLOCK) != 0);
    break;

  case ITF_NUM_CONSOLE:
    OnConsoleReceiveData(buffer, bufsize);
    break;
  }
}

//---------------------------------------------------------------------------

void cdc_task(void) {
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
