//---------------------------------------------------------------------------

#include JAVELIN_BOARD_CONFIG

#include "console_buffer.h"
#include "console_input_buffer.h"
#include "hid_keyboard_report_builder.h"
#include "javelin/debounce.h"
#include "javelin/key.h"
#include "javelin/static_allocate.h"
#include "key_state.h"
#include "plover_hid_report_buffer.h"
#include "rp2040_crc32.h"
#include "split_tx_rx.h"
#include "usb_descriptors.h"
#include "ws2812.h"

#include <stdlib.h>
#include <tusb.h>

//---------------------------------------------------------------------------

void InitJavelinSteno();
void InitSlave();
void ProcessStenoTick();
void InitMulticore();

//---------------------------------------------------------------------------

const uint64_t DEBOUNCE_TIME_US = 5000;
int resumeCount = 0;

//---------------------------------------------------------------------------
// Device callbacks
//---------------------------------------------------------------------------

// Invoked when device is mounted
extern "C" void tud_mount_cb(void) {
  ConsoleBuffer::instance.Reset();
  PloverHidReportBuffer::instance.Reset();
  HidKeyboardReportBuilder::instance.Reset();
}

// Invoked when device is unmounted
extern "C" void tud_umount_cb(void) {}

// Invoked when usb bus is suspended
// remoteWakeupEnabled: if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
extern "C" void tud_suspend_cb(bool remoteWakeupEnabled) {
  // set_sys_clock_khz(25000, true);
}

// Invoked when usb bus is resumed
extern "C" void tud_resume_cb(void) {
  // set_sys_clock_khz(125000, true);
  ++resumeCount;
}

//---------------------------------------------------------------------------
// USB HID
//---------------------------------------------------------------------------

#if JAVELIN_SPLIT
class HidTask final : public SplitRxHandler {
#else
class HidTask {
#endif
public:
  HidTask() : buttonManager(BUTTON_MANAGER_BYTE_CODE) {}

  void Update();

private:
  bool isReady = false;
#if JAVELIN_SPLIT
  bool isSplitUpdated = false;
  ButtonState splitState;
#endif
  GlobalDeferredDebounce<ButtonState> debouncer;
  ButtonManager buttonManager;

#if JAVELIN_SPLIT
  virtual void OnConnectionReset() {
    splitState.ClearAll();
    isSplitUpdated = true;
  }
  virtual void OnDataReceived(const void *data, size_t length) {
    const ButtonState &newSplitState = *(const ButtonState *)data;
    if (newSplitState == splitState) {
      return;
    }
    splitState = newSplitState;
    isSplitUpdated = true;
  }
#endif
};

void HidTask::Update() {
  buttonManager.Tick();

  Debounced<ButtonState> keyState = debouncer.Update(KeyState::Read());

#if JAVELIN_SPLIT
  if (!keyState.isUpdated && !isSplitUpdated) {
    return;
  }
  const ButtonState newKeyState = keyState.value | splitState;
  isSplitUpdated = false;
#else
  if (!keyState.isUpdated) {
    return;
  }
  const ButtonState &newKeyState = keyState.value;
#endif

  if (tud_suspended()) {
    if (newKeyState.IsAnySet()) {
      // Wake up host if we are in suspend mode
      // and REMOTE_WAKEUP feature is enabled by host
      tud_remote_wakeup();
    }
  } else if (isReady) {
    buttonManager.Update(newKeyState);
  } else if (tud_hid_n_ready(ITF_NUM_KEYBOARD)) {
    isReady = true;
    buttonManager.Update(newKeyState);
  }
}

class SlaveHidTask final : public SplitTxHandler {
public:
  void Update();
  void UpdateBuffer(TxBuffer &buffer);

private:
  GlobalDeferredDebounce<ButtonState> debouncer;
};

void SlaveHidTask::Update() {
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
  }
}

void SlaveHidTask::UpdateBuffer(TxBuffer &buffer) {
  buffer.Add(SplitHandlerId::KEY_STATE, &debouncer.GetState(),
             sizeof(ButtonState));
}

// Invoked when received SET_PROTOCOL request
// protocol is either HID_PROTOCOL_BOOT (0) or HID_PROTOCOL_REPORT (1)
extern "C" void tud_hid_set_protocol_cb(uint8_t instance, uint8_t protocol) {}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
extern "C" void tud_hid_report_complete_cb(uint8_t instance,
                                           const uint8_t *report,
                                           uint16_t length) {
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
extern "C" uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t reportId,
                                          hid_report_type_t report_type,
                                          uint8_t *buffer, uint16_t reqlen) {
  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
extern "C" void tud_hid_set_report_cb(uint8_t instance, uint8_t reportId,
                                      hid_report_type_t reportType,
                                      const uint8_t *buffer,
                                      uint16_t bufferSize) {
  switch (instance) {
  case ITF_NUM_KEYBOARD:
    if (reportType != HID_REPORT_TYPE_OUTPUT || bufferSize < 1) {
      return;
    }
    Key::SetKeyboardLedStatus(*(KeyboardLedStatus *)buffer);
    break;

  case ITF_NUM_CONSOLE:
    ConsoleInputBuffer::Add(buffer, bufferSize);
    break;
  }
}

//---------------------------------------------------------------------------

static void cdc_task() {
  uint8_t itf;

  for (itf = 0; itf < CFG_TUD_CDC; itf++) {
    if (tud_cdc_n_available(itf)) {
      uint8_t buf[64];
      uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));
      // Do nothing with it.
    }
  }
}

//---------------------------------------------------------------------------

JavelinStaticAllocate<HidTask> hidTaskContainer;
JavelinStaticAllocate<SlaveHidTask> slaveHidTaskContainer;

void DoMasterRunLoop() {
  while (1) {
    tud_task(); // tinyusb device task
    SplitTxRx::Update();
    hidTaskContainer->Update();
    cdc_task();

    ProcessStenoTick();
    ConsoleInputBuffer::Process();
    Ws2812::Update();

    sleep_us(100);
  }
}

void DoSlaveRunLoop() {
  while (1) {
    tud_task(); // tinyusb device task
    SplitTxRx::Update();
    slaveHidTaskContainer->Update();
    cdc_task();

    HidKeyboardReportBuilder::instance.FlushIfRequired();
    ConsoleBuffer::instance.Flush();
    ConsoleInputBuffer::Process();
    Ws2812::Update();

    sleep_us(100);
  }
}

int main(void) {
#if JAVELIN_THREADS
  InitMulticore();
#endif
  KeyState::Initialize();
  Rp2040Crc32::Initialize();
  Ws2812::Initialize();
  SplitTxRx::Initialize();

  // Trigger button init script.
  new (hidTaskContainer) HidTask;
  Ws2812::Update();

  if (SplitTxRx::IsMaster()) {
    SplitTxRx::RegisterRxHandler(SplitHandlerId::KEY_STATE,
                                 &hidTaskContainer.value);
    Ws2812::RegisterTxHandler();

    InitJavelinSteno();
    tusb_init();

    DoMasterRunLoop();
  } else {
    new (slaveHidTaskContainer) SlaveHidTask;

    SplitTxRx::RegisterTxHandler(&slaveHidTaskContainer.value);
    Ws2812::RegisterRxHandler();

    InitSlave();
    tusb_init();
    DoSlaveRunLoop();
  }

  return 0;
}

//---------------------------------------------------------------------------
