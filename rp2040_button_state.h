//---------------------------------------------------------------------------

#pragma once
#include "javelin/button_state.h"

//---------------------------------------------------------------------------

class Rp2040ButtonState {
public:
  static void Initialize();

  static ButtonState Read();

  static void ReadTouchCounters(uint32_t *counters);
};

//---------------------------------------------------------------------------
