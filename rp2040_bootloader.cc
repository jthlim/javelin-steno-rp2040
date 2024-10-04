//---------------------------------------------------------------------------

#include "javelin/hal/bootloader.h"
#include <stdint.h>
#include <stdlib.h>

//---------------------------------------------------------------------------

void Bootloader::Launch() {
  const uint16_t *const functionTableAddress = (const uint16_t *)0x14;
  size_t (*LookupMethod)(uint32_t table, uint32_t key) =
      (size_t(*)(uint32_t, uint32_t))(size_t)functionTableAddress[2];
  void (*UsbBoot)(int, int) = (void (*)(int, int))LookupMethod(
      functionTableAddress[0], 'U' + 'B' * 256);
  (*UsbBoot)(0, 0);
}

//---------------------------------------------------------------------------
