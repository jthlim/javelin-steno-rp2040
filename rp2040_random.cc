//---------------------------------------------------------------------------

#include "javelin/random.h"

//---------------------------------------------------------------------------

volatile uint32_t *const randomBit = (uint32_t *)0x4006001c;

uint32_t Random::GenerateHardwareUint32() {
  uint32_t result = 0;
  for (int i = 0; i < 32; ++i) {
    result = (result << 1) | (*randomBit & 1);
  }

  return result;
}

//---------------------------------------------------------------------------
