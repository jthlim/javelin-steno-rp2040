//---------------------------------------------------------------------------

#include "split_led_status.h"
#include "javelin/key.h"

//---------------------------------------------------------------------------

#if JAVELIN_SPLIT

//---------------------------------------------------------------------------

SplitLedStatus::SplitLedStatusData SplitLedStatus::instance;

void SplitLedStatus::SplitLedStatusData::UpdateBuffer(TxBuffer &buffer) {
  if (!dirty && !transmitting) {
    return;
  }
  dirty = false;
  transmitting = true;

  buffer.Add(SplitHandlerId::LED_STATUS, &value, 1);
}

void SplitLedStatus::SplitLedStatusData::OnTransmitSucceeded() {
  transmitting = false;
}

void SplitLedStatus::SplitLedStatusData::OnDataReceived(const void *data,
                                                        size_t length) {
  Key::SetKeyboardLedStatus(*(KeyboardLedStatus *)data);
}

//---------------------------------------------------------------------------

#endif // JAVELIN_SPLIT

//---------------------------------------------------------------------------
