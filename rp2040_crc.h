//---------------------------------------------------------------------------

#pragma once
#include <stdint.h>
#include <stdlib.h>

//---------------------------------------------------------------------------

struct Rp2040Crc {
public:
  static void Initialize();

  static uint32_t Crc32(const void *data, size_t length);
  static uint32_t Crc16Ccitt(const void *data, size_t length);
};

//---------------------------------------------------------------------------
