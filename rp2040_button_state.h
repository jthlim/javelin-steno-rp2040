//---------------------------------------------------------------------------

#pragma once
#include "javelin/script_manager.h"

//---------------------------------------------------------------------------

class Rp2040ButtonState {
public:
  static void Initialize();

  static ButtonState Read();

  static void ReadTouchCounters(uint32_t *counters);
};

//---------------------------------------------------------------------------
