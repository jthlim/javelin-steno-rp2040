//---------------------------------------------------------------------------

#pragma once
#include "pico_register.h"

//---------------------------------------------------------------------------

struct PicoInterpolator {
  PicoRegister accumulator[2];
  PicoRegister base[3];
  const PicoRegister pop[3];
  const PicoRegister peek[3];
  PicoRegister control[2];
  PicoRegister accumulatorAdd[2];
  PicoRegister bases; // Lower 16 bits to base0, Upper 16 to base1
};

static_assert(sizeof(PicoInterpolator) == 0x40);

PicoInterpolator *const interpolator0 = (PicoInterpolator *)0xd0000080;
PicoInterpolator *const interpolator1 = (PicoInterpolator *)0xd00000c0;

//---------------------------------------------------------------------------
