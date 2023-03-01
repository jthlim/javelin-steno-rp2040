//---------------------------------------------------------------------------

#include "split_led_status.h"
#include "javelin/key.h"

//---------------------------------------------------------------------------

#if JAVELIN_SPLIT

//---------------------------------------------------------------------------

SplitLedStatus::SplitLedStatusData SplitLedStatus::instance;

void SplitLedStatus::SplitLedStatusData::UpdateBuffer(TxBuffer &buffer) {
  if (!dirty) {
    return;
  }
  dirty = false;
  buffer.Add(SplitHandlerId::LED_STATUS, &value, 1);
}

void SplitLedStatus::SplitLedStatusData::OnDataReceived(const void *data,
                                                        size_t length) {
  Key::SetKeyboardLedStatus(*(KeyboardLedStatus *)data);
}

//---------------------------------------------------------------------------

#endif // JAVELIN_SPLIT

//---------------------------------------------------------------------------
