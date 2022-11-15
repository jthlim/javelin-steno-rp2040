//---------------------------------------------------------------------------

#pragma once
#include "tusb_config.h"

//---------------------------------------------------------------------------

enum {
  ITF_NUM_KEYBOARD,
  ITF_NUM_CONSOLE,
#if USE_PLOVER_HID
  ITF_NUM_PLOVER_HID,
#endif
  ITF_NUM_CDC,
  ITF_NUM_CDC_DATA,
  ITF_NUM_TOTAL,
};

//---------------------------------------------------------------------------
