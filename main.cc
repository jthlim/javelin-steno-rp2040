//---------------------------------------------------------------------------

#include JAVELIN_BOARD_CONFIG

#include "console_report_buffer.h"
#include "hid_keyboard_report_builder.h"
#include "javelin/console_input_buffer.h"
#include "javelin/debounce.h"
#include "javelin/flash.h"
#include "javelin/hal/bootloader.h"
#include "javelin/key.h"
#include "javelin/keyboard_led_status.h"
#include "javelin/split/pair_console.h"
#include "javelin/split/split_serial_buffer.h"
#include "javelin/split/split_usb_status.h"
#include "javelin/static_allocate.h"
#include "javelin/timer_manager.h"
#include "plover_hid_report_buffer.h"
#include "rp2040_button_state.h"
#include "rp2040_crc.h"
#include "rp2040_split.h"
#include "rp2040_ws2812.h"
#include "split_hid_report_buffer.h"
#include "ssd1306.h"
#include "usb_descriptors.h"

#include <hardware/watchdog.h>
#include <pico/time.h>
#include <tusb.h>

//---------------------------------------------------------------------------

void InitJavelinMaster();
void InitJavelinSlave();
void ProcessStenoTick();
void InitMulticore();

//---------------------------------------------------------------------------

#if JAVELIN_USE_WATCHDOG
extern uint32_t watchdogData[8];
#endif

//---------------------------------------------------------------------------
// Device callbacks
//---------------------------------------------------------------------------

// Invoked when device is mounted
extern "C" void tud_mount_cb(void) {
  ConsoleReportBuffer::instance.Reset();
  PloverHidReportBuffer::instance.Reset();
  HidKeyboardReportBuilder::instance.Reset();
  SplitUsbStatus::instance.OnMount();
  SplitUsbStatus::instance.SetPowered(true);
  ScriptManager::ExecuteScript(ScriptId::CONNECTION_UPDATE);
}

// Invoked when device is unmounted
extern "C" void tud_umount_cb(void) {
  SplitUsbStatus::instance.OnUnmount();
  SplitUsbStatus::instance.SetPowered(false);
  ScriptManager::ExecuteScript(ScriptId::CONNECTION_UPDATE);
}

// Invoked when usb bus is suspended
// remoteWakeupEnabled: if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
extern "C" void tud_suspend_cb(bool remoteWakeupEnabled) {
  // set_sys_clock_khz(25000, true);
  SplitUsbStatus::instance.OnSuspend();
}

// Invoked when usb bus is resumed
extern "C" void tud_resume_cb(void) {
  // set_sys_clock_khz(125000, true);
  SplitUsbStatus::instance.OnResume();
}

//---------------------------------------------------------------------------
// USB HID
//---------------------------------------------------------------------------

#if JAVELIN_SPLIT
class MasterTask final : public SplitRxHandler {
#else
class MasterTask {
#endif
public:
  void Update();

private:
#if JAVELIN_SPLIT
  ButtonState splitState;
#endif
  GlobalDeferredDebounce<ButtonState> debouncer;

#if JAVELIN_SPLIT
  virtual void OnReceiveConnectionReset() { splitState.ClearAll(); }
  virtual void OnDataReceived(const void *data, size_t length) {
    const ButtonState &newSplitState = *(const ButtonState *)data;
    splitState = newSplitState;
  }
#endif
};

void MasterTask::Update() {
  if (Flash::IsUpdating()) {
    return;
  }

  uint32_t scriptTime = Clock::GetMilliseconds();
  ScriptManager::GetInstance().Tick(scriptTime);
  TimerManager::instance.ProcessTimers(scriptTime);

#if JAVELIN_SPLIT
  Debounced<ButtonState> buttonState =
      debouncer.Update(Rp2040ButtonState::Read() | splitState);
#else
  Debounced<ButtonState> buttonState =
      debouncer.Update(Rp2040ButtonState::Read());
#endif
  if (!buttonState.isUpdated) {
    return;
  }

  if (tud_suspended()) {
    if (buttonState.value.IsAnySet()) {
      // Wake up host if we are in suspend mode
      // and REMOTE_WAKEUP feature is enabled by host
      tud_remote_wakeup();
    }
  }

  ScriptManager::GetInstance().Update(buttonState.value,
                                      Clock::GetMilliseconds());
}

class SlaveTask final : public SplitTxHandler {
public:
  void Update();
  void UpdateBuffer(TxBuffer &buffer);

private:
  bool needsTransmit = true;
  ButtonState buttonState;

  virtual void OnTransmitConnectionReset() { needsTransmit = true; }
};

void SlaveTask::Update() {
  if (Flash::IsUpdating()) {
    return;
  }

  uint32_t scriptTime = Clock::GetMilliseconds();
  ScriptManager::GetInstance().Tick(scriptTime);
  TimerManager::instance.ProcessTimers(scriptTime);

  ButtonState newButtonState = Rp2040ButtonState::Read();
  if (newButtonState == buttonState) {
    return;
  }

  buttonState = newButtonState;
  needsTransmit = true;

  if (tud_suspended()) {
    if (buttonState.IsAnySet()) {
      // Wake up host if we are in suspend mode
      // and REMOTE_WAKEUP feature is enabled by host
      tud_remote_wakeup();
    }
  }
}

void SlaveTask::UpdateBuffer(TxBuffer &buffer) {
  if (needsTransmit) {
    needsTransmit = false;
    buffer.Add(SplitHandlerId::KEY_STATE, &buttonState, sizeof(ButtonState));
  }
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
    ConsoleReportBuffer::instance.SendNextReport();
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
    if (reportType != HID_REPORT_TYPE_OUTPUT ||
        reportId != KEYBOARD_PAGE_REPORT_ID || bufferSize < 1) {
      return;
    }
    SplitUsbStatus::instance.SetKeyboardLedStatus(
        *(KeyboardLedStatus *)&buffer[0]);
    break;

  case ITF_NUM_CONSOLE:
    ConsoleInputBuffer::Add(buffer, bufferSize, ConnectionId::USB);
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

JavelinStaticAllocate<MasterTask> masterTaskContainer;
JavelinStaticAllocate<SlaveTask> slaveTaskContainer;

void DoMasterRunLoop() {
#if JAVELIN_USE_WATCHDOG
  watchdog_enable(1000, true);
#endif

  while (1) {
    tud_task(); // tinyusb device task
    masterTaskContainer->Update();
    Rp2040Split::Update();
    cdc_task();

    ProcessStenoTick();
    ConsoleInputBuffer::Process();
    Ws2812::Update();
    Ssd1306::Update();

#if JAVELIN_USE_WATCHDOG
    watchdog_update();
#endif
    sleep_us(100);
  }
}

void DoSlaveRunLoop() {
#if JAVELIN_USE_WATCHDOG
  watchdog_enable(1000, true);
#endif

  while (1) {
    tud_task(); // tinyusb device task
    slaveTaskContainer->Update();
    Rp2040Split::Update();
    cdc_task();

    SplitHidReportBuffer::Update();
    HidKeyboardReportBuilder::instance.FlushIfRequired();
    ConsoleReportBuffer::instance.Flush();
    ConsoleInputBuffer::Process();
    Ws2812::Update();
    Ssd1306::Update();
    PairConsole::Process();

#if JAVELIN_USE_WATCHDOG
    watchdog_update();
#endif
    sleep_us(100);
  }
}

int main(void) {
#if JAVELIN_USE_WATCHDOG
  for (int i = 0; i < 8; ++i) {
    watchdogData[i] = watchdog_hw->scratch[i];
  }
#endif

#if JAVELIN_THREADS
  InitMulticore();
#endif
  Rp2040ButtonState::Initialize();
  Rp2040Crc::Initialize();
  Ws2812::Initialize();
  Rp2040Split::Initialize();
  Ssd1306::Initialize();

  if (Split::IsMaster()) {
    new (masterTaskContainer) MasterTask;

    Split::RegisterRxHandler(SplitHandlerId::KEY_STATE,
                             &masterTaskContainer.value);
    ConsoleInputBuffer::RegisterRxHandler();
    Ws2812::RegisterTxHandler();
    SplitHidReportBuffer::RegisterMasterHandlers();
    SplitSerialBuffer::RegisterTxHandler();
    Ssd1306::RegisterMasterHandlers();
    SplitUsbStatus::RegisterHandlers();
    PairConsole::RegisterHandlers();

    InitJavelinMaster();
    ScriptManager::Initialize(SCRIPT_BYTE_CODE);
    tusb_init();

    DoMasterRunLoop();
  } else {
    new (slaveTaskContainer) SlaveTask;

    Split::RegisterTxHandler(&slaveTaskContainer.value);
    ConsoleInputBuffer::RegisterTxHandler();
    Ws2812::RegisterRxHandler();
    SplitHidReportBuffer::RegisterSlaveHandlers();
    SplitSerialBuffer::RegisterRxHandler();
    Ssd1306::RegisterSlaveHandlers();
    SplitUsbStatus::RegisterHandlers();
    PairConsole::RegisterHandlers();

    InitJavelinSlave();
    ScriptManager::Initialize(SCRIPT_BYTE_CODE);

    tusb_init();
    DoSlaveRunLoop();
  }

  return 0;
}

//---------------------------------------------------------------------------
